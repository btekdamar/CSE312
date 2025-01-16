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
		jal linearSearch
		li $v0, 4
		la $a0, find
		syscall
		li $v0, 10
		syscall
		
	linearSearch:
		loop:
			beq $t1, $t3, exit
			sll $t4, $t1, 2
			addu $t4, $t4, $s0
			lw $t5, 0($t4)
			beq $t0, $t5, flag
			addi $t1, $t1, 1
			b loop 
		exit:
			li $v0, 4
			la $a0, notFind
			syscall
			li $v0, 10
			syscall
		
	flag:
		jr $ra
