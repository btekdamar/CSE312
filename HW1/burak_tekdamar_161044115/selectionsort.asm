.data
	arr: .word 5 1 8 3 7 213 12 200 322 20 22 14 16 15 29 40 25 300 13 43
	arrSize: .word 20
	space: .asciiz " "
	nonSorted: .asciiz "Before sort\n"
	sorted: .asciiz "\nAfter sort\n"
	
.text
.globl main
	main:
		la $s0, arr
		jal nonSortedArr
		add $t0, $zero, $zero #i index
		lw $t1, arrSize #array size
		addi $t2, $t1, -1 #arraysize - 1
		jal selectionSort
		j printArr
		
	selectionSort:		
		loop1:
			beq $t0, $t2, exit1
			add $t3, $t0, $zero
			sll $t7, $t3, 2
			addu $t7, $t7, $s0
			lw $s1, 0($t7)
			add $t4, $t0, $zero
			loop2:
				beq $t4, $t1, exit2
				sll $t5, $t4, 2
				addu $t5, $t5, $s0
				lw $t6, 0($t5)
				blt $t6, $s1, changeMin
				addi $t4, $t4, 1
				j loop2
			exit2:
				sll $s2, $t0, 2
				addu $s2, $s2, $s0
				lw $s7, 0($s2)
				sll $t7, $t3, 2
				addu $t7, $t7, $s0
				lw $s6, 0($t7)
				sw $s6, 0($s2)
				sw $s7, 0($t7)
				addi $t0, $t0, 1
				j loop1
		exit1:
			jr $ra
		
	changeMin:
		add $t3, $t4, $zero
		sll $t7, $t3, 2
		addu $t7, $t7, $s0
		lw $s1, 0($t7)
		addi $t4, $t4, 1
		j loop2
		
	nonSortedArr:
		add $t0, $zero, $zero
		lw $t1, arrSize
		addi $t2, $t1, -1
		li $v0, 4
		la $a0, nonSorted
		syscall
		add $t0, $zero, $zero
		loopNonSorted:
			beq $t0, $t1, exitNonSorted
			sll $s5, $t0, 2
			addu $s5, $s5, $s0
			lw $s6, 0($s5)
			li $v0, 1
			la $a0, ($s6)
			syscall
			li $v0, 4
			la $a0, space
			syscall
			addi $t0, $t0, 1
			j loopNonSorted
		exitNonSorted:
			jr $ra
			
	printArr:
		li $v0, 4
		la $a0, sorted
		syscall
		add $t0, $zero, $zero
		loop:
			beq $t0, $t1, exit
			sll $s5, $t0, 2
			addu $s5, $s5, $s0
			lw $s6, 0($s5)
			li $v0, 1
			la $a0, ($s6)
			syscall
			li $v0, 4
			la $a0, space
			syscall
			addi $t0, $t0, 1
			j loop
		exit:
			li $v0, 10
			syscall
