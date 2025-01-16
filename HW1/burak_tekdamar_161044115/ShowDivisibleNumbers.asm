.data
	promt: .asciiz "Please enter number:"
	space: .asciiz " "
.text
.globl main
	main:
		jal readNumbers
		jal findMin
		loop:
			bgt $t0, $t1, exit
			div $t4, $t0, $t2
			mfhi $s1
			beq $s1, $zero, printNumber
			addi $t0, $t0, 1
			j loop
		exit:
			li $v0, 10
			syscall
		
	readNumbers:
		li $v0, 4
		la $a0, promt
		syscall
		li $v0, 5
		syscall
		add $t0, $v0, $zero
		li $v0, 4
		la $a0, promt
		syscall
		li $v0, 5
		syscall
		add $t1, $v0, $zero
		li $v0, 4
		la $a0, promt
		syscall
		li $v0, 5
		syscall
		add $t2, $v0, $zero
		jr $ra
		
	findMin:
		blt $t1, $t0, changeNumber
		jr $ra
		
	changeNumber:
		add $t3, $t1, $zero
		add $t1, $t0, $zero
		add $t0, $t3, $zero
		jr $ra 
		
	printNumber:
		li $v0, 1
		la $a0, ($t0)
		syscall 
		li $v0, 4
		la $a0, space
		syscall 
		addi $t0, $t0, 1
		j loop
