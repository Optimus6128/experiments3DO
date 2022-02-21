; void vramSet(uint32 c, void* bufferDst)

    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    EXPORT vramSet

vramSet
	stmfd sp!, {r4-r11, lr}

	mov r4,r0
	mov r5,r0
	mov r6,r0
	mov r7,r0
	mov r8,r0
	mov r9,r0
	mov r10,r0
	mov r11,r0

	mov r3,r1

	mov r1,#600
loopVramSet
		stmia r3!,{r4-r11}
		stmia r3!,{r4-r11}
		stmia r3!,{r4-r11}
		stmia r3!,{r4-r11}
		stmia r3!,{r4-r11}
		stmia r3!,{r4-r11}
		stmia r3!,{r4-r11}
		stmia r3!,{r4-r11}
	subs r1,r1,#1
	bne loopVramSet

	ldmfd sp!, {r4-r11, pc}

	END
