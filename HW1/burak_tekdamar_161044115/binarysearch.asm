.data
	arr: .word 1 5 9 12 16 20 25 29 30 40
	arrSize: .word 10
	find: .asciiz "Number found\n"
	notFind: .asciiz "Number not found\n"
	promt: .asciiz "Please enter your number:"
.text
.globl main
	main:
		la $s0, arr
		li $v0, 4
		la $a0, promt
		syscall
		li $v0, 5
		syscall
		move $t0, $v0
		add $t1, $zero, $zero
		addi $t2, $zero, -1
		lw $t3, arrSize
		jal binarySearch
		li $v0, 4
		la $a0, find
		syscall
		li $v0, 10
		syscall
		
	
	binarySearch:
		loop:
			sub $t4, $t3, $t2
			ble $t4, 1, exit
			add $t5, $t3, $t2
			div $t5, $t5, 2
			sll $t6, $t5, 2
			addu $t6, $t6, $s0
			lw $t7, 0($t6)
			beq $t7, $t0, flag
			blt $t7, $t0, right
			b left
		exit:
			li $v0, 4
			la $a0, notFind
			syscall
			li $v0, 10
			syscall
			
	flag:
		jr $ra
			 
	left:
		add $t3, $t5, $zero
		j loop
	
	right:
		add $t2, $t5, $zero
		j loop
		
		
