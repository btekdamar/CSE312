.data
	message1: .asciiz "1: ShowDivisibleNumbers.asm\n"
	message2: .asciiz "2: selectionsort.asm\n"
	message3: .asciiz "3: binarysearch.asm\n"
	message4: .asciiz "4: linearsearch.asm\n"
	buffer: .space 256
.text
.globl main
	main:
		loopMenu:
			li $v0, 4
			la $a0, message1
			syscall
			li $v0, 4
			la $a0, message2
			syscall
			li $v0, 4
			la $a0, message3
			syscall
			li $v0, 4
			la $a0, message4
			syscall
			li $v0, 8
			la $a0, buffer
			li $a1, 256
			syscall
			li $v0, 18
			syscall
			j loopMenu
			
