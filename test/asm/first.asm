; incremental counter example

main:
	XOR R0, R0
.loop:
	ADDC R0, 1
	MOV R1, R0
	BRANCH .loop