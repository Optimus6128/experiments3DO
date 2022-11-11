; void FLIupdateFullFrame(uint16 *dst, uint32 *vga_pal, uint32 *vga32)

    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    EXPORT FLIupdateFullFrame

FLIupdateFullFrame
	stmfd sp!, {r4-r12, lr}

	mov r14,#255
	mov r4,#4000
loopFliUpdate
	ldmia r2!,{r5-r8}


	mov r9,r5,lsr #24
	and r10,r14,r5,lsr #16
	and r11,r14,r5,lsr #8
	and r12,r14,r5

	ldr r9,[r1,r9,lsl #2]
	ldr r10,[r1,r10,lsl #2]
	ldr r11,[r1,r11,lsl #2]
	ldr r12,[r1,r12,lsl #2]

	orr r9,r9,r10,lsr #16
	orr r10,r11,r12,lsr #16
	
	stmia r0!,{r9-r10}


	mov r9,r6,lsr #24
	and r10,r14,r6,lsr #16
	and r11,r14,r6,lsr #8
	and r12,r14,r6

	ldr r9,[r1,r9,lsl #2]
	ldr r10,[r1,r10,lsl #2]
	ldr r11,[r1,r11,lsl #2]
	ldr r12,[r1,r12,lsl #2]

	orr r9,r9,r10,lsr #16
	orr r10,r11,r12,lsr #16
	
	stmia r0!,{r9-r10}


	mov r9,r7,lsr #24
	and r10,r14,r7,lsr #16
	and r11,r14,r7,lsr #8
	and r12,r14,r7

	ldr r9,[r1,r9,lsl #2]
	ldr r10,[r1,r10,lsl #2]
	ldr r11,[r1,r11,lsl #2]
	ldr r12,[r1,r12,lsl #2]

	orr r9,r9,r10,lsr #16
	orr r10,r11,r12,lsr #16
	
	stmia r0!,{r9-r10}


	mov r9,r8,lsr #24
	and r10,r14,r8,lsr #16
	and r11,r14,r8,lsr #8
	and r12,r14,r8

	ldr r9,[r1,r9,lsl #2]
	ldr r10,[r1,r10,lsl #2]
	ldr r11,[r1,r11,lsl #2]
	ldr r12,[r1,r12,lsl #2]

	orr r9,r9,r10,lsr #16
	orr r10,r11,r12,lsr #16
	
	stmia r0!,{r9-r10}



	subs r4,r4,#1
	bgt loopFliUpdate

	ldmfd sp!, {r4-r12, pc}

	END
