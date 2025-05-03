; .memorymap
;    defaultslot     2
;    ; ROM area
;    slotsize        $2000
;    slot            0       $0000
;    slot            1       $2000
;    slot            2       $4000
;    slot            3       $6000
;    slot            4       $8000
;    slot            5       $A000
;    slot            6       $C000
;    slot            7       $E000
; .endme

.define vce_crLo $400
.define vce_crHi $401
.define vce_ctaLo $402
.define vce_ctaHi $403
.define vce_ctwLo $404
.define vce_ctwHi $405

.define vdp_select $0000
.define vdp_data   $0002
  .define vdp_dataLo $0002
  .define vdp_dataHi $0003

.define vdp_reg_mawr $00
.define vdp_reg_marr $01
.define vdp_reg_vrr  $02
.define vdp_reg_vwr  $02
.define vdp_reg_cr   $05

.define tile_pat_size $20
.define sprite_pat_size $80

.define sprite_xbase $20

.define lines_per_palette $10
.define palette_line_size $20

.define slotSize $2000
