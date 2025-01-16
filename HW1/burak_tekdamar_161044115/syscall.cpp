/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"
#include "spim-utils.h"

struct memBackUp
{
	reg_word R_temp[R_LENGTH];
	reg_word HI_temp, LO_temp;
	mem_addr PC_temp, nPC_temp;
	double *FPR_temp;			
	float *FGR_temp;			
	int *FWR_temp;			
	reg_word CCR_temp[4][32], CPR_temp[4][32];
	instruction **text_seg_temp;
	bool text_modified_temp;	
	mem_addr text_top_temp;
	mem_word *data_seg_temp;
	bool data_modified_temp;	
	short *data_seg_h_temp;	
	BYTE_TYPE *data_seg_b_temp;
	mem_addr data_top_temp;
	mem_addr gp_midpoint_temp;
	mem_word *stack_seg_temp;
	short *stack_seg_h_temp;
	BYTE_TYPE *stack_seg_b_temp;
	mem_addr stack_bot_temp;
	instruction **k_text_seg_temp;
	mem_addr k_text_top_temp;
	mem_word *k_data_seg_temp;
	short *k_data_seg_h_temp;
	BYTE_TYPE *k_data_seg_b_temp;
	mem_addr k_data_top_temp;

};

#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>

void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
  if (function != NULL)
    {
      run_error ("Bad parameter to system call: %s\n", function);
    }
  else
    {
      run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
  static _invalid_parameter_handler oldHandler;
  static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

  if (flag == 0)
    {
      oldHandler = _set_invalid_parameter_handler(newHandler);
      _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
  else
    {
      newHandler = _set_invalid_parameter_handler(oldHandler);
      _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif


/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */

int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif

  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      write_output (console_out, "%d", R[REG_A0]);
      break;

    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (REG_FA0);

	write_output (console_out, "%.8f", val);
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      break;

    case PRINT_STRING_SYSCALL:
      write_output (console_out, "%s", mem_reference (R[REG_A0]));
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	R[REG_RES] = atol (str);
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR_S (REG_FRES) = (float) atof (str);
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR [REG_FRES] = atof (str);
	break;
      }

    case READ_STRING_SYSCALL:
      {
	read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
	data_modified = true;
	break;
      }

    case SBRK_SYSCALL:
      {
	mem_addr x = data_top;
	expand_data (R[REG_A0]);
	R[REG_RES] = x;
	data_modified = true;
	break;
      }

    case PRINT_CHARACTER_SYSCALL:
      write_output (console_out, "%c", R[REG_A0]);
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];

	read_input (str, 2);
	if (*str == '\0') *str = '\n';      /* makes xspim = spim */
	R[REG_RES] = (long) str[0];
	break;
      }

    case EXIT_SYSCALL:
      spim_return_value = 0;
      return (0);

    case EXIT2_SYSCALL:
      spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
      return (0);

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
	R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
	break;
      }

    case READ_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	data_modified = true;
	break;
      }

    case WRITE_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
	R[REG_RES] = _close(R[REG_A0]);
#else
	R[REG_RES] = close(R[REG_A0]);
#endif
	break;
      }

    case CREATE_PROCESS_SYSCALL:
    {
    	static bool load_exception_handler = true;
    	bool continuable;
    	char *source = (char *)mem_reference (R[REG_A0]);
    	memBackUp tempMemory;

    	for(int i = 0; source[i] != '\0'; ++i){
    		if(source[i] != '\n')
    			source[i] = source[i];
    		else if(source[i] == '\n')
    			source[i] = '\0';
    	}

    	char *path = strdup(source);

		int i = 0;
		int j = 0;
		while(i < R_LENGTH){
			tempMemory.R_temp[i] = R[i];
			++i;
		}

		i = 0;
		while(i < 4){
			while(j < 32){
				tempMemory.CCR_temp[i][j] = CCR[i][j];
				tempMemory.CPR_temp[i][j] = CPR[i][j];
				++j;
			}
			++i;
		}

		tempMemory.HI_temp = HI;
		tempMemory.LO_temp = LO;
		tempMemory.PC_temp = PC;
		tempMemory.nPC_temp = nPC;

		tempMemory.FPR_temp = FPR;			
		tempMemory.FGR_temp = FGR;			
		tempMemory.FWR_temp = FWR;		

		tempMemory.text_seg_temp = text_seg;
		tempMemory.text_modified_temp = text_modified;	
		tempMemory.text_top_temp = text_top;
		tempMemory.data_seg_temp = data_seg;
		tempMemory.data_modified_temp = data_modified;	
		tempMemory.data_seg_h_temp = data_seg_h;	
		tempMemory.data_seg_b_temp = data_seg_b;
		tempMemory.data_top_temp = data_top;
		tempMemory.gp_midpoint_temp = gp_midpoint;
		tempMemory.stack_seg_temp = stack_seg;
		tempMemory.stack_seg_h_temp = stack_seg_h;
		tempMemory.stack_seg_b_temp = stack_seg_b;
		tempMemory.stack_bot_temp = stack_bot;
		tempMemory.k_text_seg_temp = k_text_seg;
		tempMemory.k_text_top_temp = k_text_top;
		tempMemory.k_data_seg_temp = k_data_seg;
		tempMemory.k_data_seg_h_temp = k_data_seg_h;
		tempMemory.k_data_seg_b_temp = k_data_seg_b;
		tempMemory.k_data_top_temp = k_data_top;

		FPR = NULL;
        FGR = NULL;	
        FWR = NULL;	
        text_seg = NULL;
        k_text_seg = NULL;

	    initialize_world(load_exception_handler ? exception_file_name : NULL, true);
	    initialize_stack(path);
	    read_assembly_file(path);
	    run_program(find_symbol_address (DEFAULT_RUN_LOCATION), DEFAULT_RUN_STEPS, false, false, &continuable);
	    initialize_world(load_exception_handler ? exception_file_name : NULL, true);

		i = 0;
		while(i < R_LENGTH){
			R[i] = tempMemory.R_temp[i];
			++i;
		}

		i = 0;
		j = 0;
		while(i < 4){
			while(j < 32){
				CCR[i][j] = tempMemory.CCR_temp[i][j];
				CPR[i][j] = tempMemory.CPR_temp[i][j];
				++j;
			}
			++i;
		}

		text_seg = tempMemory.text_seg_temp;
		text_modified = tempMemory.text_modified_temp;
		text_top = tempMemory.text_top_temp;
		data_seg = tempMemory.data_seg_temp;
		data_modified = tempMemory.data_modified_temp;	
		data_seg_h = tempMemory.data_seg_h_temp;		
		data_seg_b = tempMemory.data_seg_b_temp;	
		data_top = tempMemory.data_top_temp;
		gp_midpoint = tempMemory.gp_midpoint_temp;	
		stack_seg = tempMemory.stack_seg_temp;
		stack_seg_h = tempMemory.stack_seg_h_temp;
		stack_seg_b = tempMemory.stack_seg_b_temp;
		stack_bot = tempMemory.stack_bot_temp;
		k_text_seg = tempMemory.k_text_seg_temp;
		k_text_top = tempMemory.k_text_top_temp;
		k_data_seg = tempMemory.k_data_seg_temp;
		k_data_seg_h = tempMemory.k_data_seg_h_temp;
		k_data_seg_b = tempMemory.k_data_seg_b_temp;
		k_data_top = tempMemory.k_data_top_temp;

		PC = tempMemory.PC_temp;
		nPC = tempMemory.nPC_temp;
        HI = tempMemory.HI_temp;
		LO = tempMemory.LO_temp;
		FPR = tempMemory.FPR_temp;		
		FGR = tempMemory.FGR_temp;		
		FWR = tempMemory.FWR_temp;

	    break;
    }

    default:
      run_error ("Unknown system call: %d\n", R[REG_V0]);
      break;
    }

#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
  return (1);
}


void
handle_exception ()
{
  if (!quiet && CP0_ExCode != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
	error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_AdES:
      if (!quiet)
	error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_IBE:
      if (!quiet)
	error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_DBE:
      if (!quiet)
	error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_Sys:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
	error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
	error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
	error ("  Floating point\n");
      break;

    default:
      if (!quiet)
	error ("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}
