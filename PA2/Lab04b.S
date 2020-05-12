.globl Lab04b

Lab04b:
	SUB SP, SP, #32 // decrement stack pointer by 32 bytes
	STUR X1,[SP, #16] //Store param in stack
	STUR X30, [SP, #0]//Store return address in stack

	CBNZ  X0, B1 //exit if X0==0, otherwise branch to B1
	ADDI X0, XZR, #1 //return 1
	B END

B1:
	ADD X1, X0, XZR //store X0 into X1
	SUB X0,X0,#1//decrement X0
	BL Lab04b//recursion
	BL my_mul //start multiplying your way up the recursion calls.

END:
	LDUR X30, [SP, #0]//reload return address
	LDUR X1, [SP, #16]//pull from Stack
	ADD SP,SP,#32//release Stack pointer
	BR X30
