; Really simple & stupid PICA200 shader
; Taken from picasso example

; Uniforms
.fvec projection[4]

; Constants
.constf consts1(0.0, 1.0, 2.0, 0.5)

; Outputs : here only position and color
.out out_pos position
.out out_tc0 texcoord0

; Inputs : here we have only vertices
.alias in_pos v0
.alias in_tc0 v1

.proc main
	mov r0, in_pos
	
	; outpos = projection * r1
	dp4 out_pos.x, projection[0], r0
	dp4 out_pos.y, projection[1], r0
	dp4 out_pos.z, projection[2], r0
	dp4 out_pos.w, projection[3], r0

	; Set vertex c olor
	mov out_tc0, in_tc0
	end
.end
