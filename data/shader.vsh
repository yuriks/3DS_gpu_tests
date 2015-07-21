; Really simple & stupid PICA200 shader
; Taken from picasso example

; Uniforms
.fvec projection[4]

; Constants
.constf consts1(0.0, 1.0, 2.0, 0.5)
.constf consts2(-4.0, 0.5, 0.0, 0.0)
.alias zeros consts1.xxxx
.alias ones consts1.yyyy ; (1.0,1.0,1.0,1.0)

; Outputs : here only position and color
.out outpos position
.out outclr color
.out outtex0 texcoord0
.out outtex1 texcoord1
.out outtex2 texcoord2

; Inputs : here we have only vertices
.alias inpos v0
.alias incolor v1
.alias intex0 v2

.proc main
	; r0 = (inpos.x, inpos.y, inpos.z, 1.0)
	mov r0.xyz, inpos
	mov r0.w, ones
	
	; outpos = projection * r1
	dp4 outpos.x, projection[0], r0
	dp4 outpos.y, projection[1], r0
	dp4 outpos.z, projection[2], r0
	dp4 outpos.w, projection[3], r0

	mov outtex0, intex0
	mov outtex1, intex0
	mov outtex2, intex0
	; Set vertex color to white rgba => (1.0,1.0,1.0,1.0)
	add r0, consts2.xxxx, incolor
	mul outclr, consts2.yyyy, r0
	end
.end
