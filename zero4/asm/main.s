.include "include/pce_sys.inc"
.include "gen/metabanks.inc"

;==============================================================================
; debug flags
;==============================================================================

.define DEBUG_ENABLEFASTFORWARD 0

.define DEBUG_BOOT_TO_CREDITS 0

;==============================================================================
; basic configuration
;==============================================================================

.emptyfill $FF

; stiiiiiiill waiting on those dynamic bank sizes

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

.memorymap
   defaultslot     0
   
   slotsize        $10000
   slot            0       $0000
.endme

; everything goes in the only slot
.slot 0

; .rombankmap
; ;  bankstotal $30
;   bankstotal $40
;   
;   banksize $2000
; ;  banks $30
;   banks $40
; .endro

.rombankmap
  bankstotal numMetabanks
  
  banksize $10000
  banks numMetabanks
.endro

; ROMNAME should be defined via the -D argument in the wla-dx invocation
.background ROMNAME

;==============================================================================
; top-level defines
;==============================================================================

;============================
; textscript
;============================

; ; new command to jump to new text bank
; .define textop_jumpNewText $17
; ; original script jump command
; .define textop_jump $18
; .define textop_call $19

; TODO: do we also need a "call new subtext" op?
; as it is, any call to an entirely new (not in original game) piece of text
; would have to point to a "jump to new text" command in the original text bank.
; i'm thinking this won't be common enough to be worth bothering with, though.

;============================
; gamescript
;============================

.define jumpToFollowingGamescript $F973

; also $15, $16, $22
.define gamescript_op_nop $14
.macro gamescript_nop
  .db gamescript_op_nop
.endm

.define gamescript_op_openWindow $1C
.macro gamescript_openWindow ARGS slot,yPos,xPos,h,w
  .db gamescript_op_openWindow
    .db slot
    .db yPos
    .db xPos
    .db h
    .db w
.endm

.define gamescript_op_waitForTextDone $1D
.macro gamescript_waitForTextDone
  .db gamescript_op_waitForTextDone
.endm

.define gamescript_op_runText $1E
.macro gamescript_runText ARGS slot,ptr
  .db gamescript_op_runText
    .db slot
    .dw ptr
.endm

.define gamescript_op_call $21
.macro gamescript_call ARGS ptr
  .db gamescript_op_call
    .dw ptr
.endm

.define gamescript_op_callNativeCode $23
.macro gamescript_callNativeCode ARGS ptr
  .db gamescript_op_callNativeCode
    .dw ptr
.endm

.define gamescript_op_runTextIndirect $4C
.macro gamescript_runTextIndirect ARGS slot,ptr
  .db gamescript_op_runTextIndirect
    .db slot
    .dw ptr
.endm

.define gamescript_op_jump $30
.macro gamescript_jump ARGS dst
  .db gamescript_op_jump
    .dw dst
.endm

.define gamescript_op_writeByte $31
.macro gamescript_writeByte ARGS dst,value
  .db gamescript_op_writeByte
    .dw dst
    .db value
.endm

;============================
; misc
;============================

.define metabankSize $10000

;==============================================================================
; top-level macros
;==============================================================================

;============================
; math
;============================

.macro aslByPowerOf2 ARGS multiplier
  .redefine aslByPowerOf2_loopCount (log(multiplier)/log(2))
  .redefine aslByPowerOf2_loopCountCheck floor(aslByPowerOf2_loopCount)
  
  .if (aslByPowerOf2_loopCount != aslByPowerOf2_loopCountCheck)
    .print "aslByPowerOf2: argument (\1) is not a power of 2\n"
    .fail
  .endif
  
  .rept aslByPowerOf2_loopCount
    asl
  .endr
.endm

.macro lsrByPowerOf2 ARGS multiplier
  .redefine lsrByPowerOf2_loopCount (log(multiplier)/log(2))
  .redefine lsrByPowerOf2_loopCountCheck floor(aslByPowerOf2_loopCount)
  
  .if (lsrByPowerOf2_loopCount != lsrByPowerOf2_loopCountCheck)
    .print "lsrByPowerOf2: argument (\1) is not a power of 2\n"
    .fail
  .endif
  
  .rept lsrByPowerOf2_loopCount
    lsr
  .endr
.endm

;============================
; metabanking
;============================

.macro unbackgroundMetabank ARGS name, start, end
;   .redefine test (metabank_\1*metabankSize)+start
;   .redefine test2 (metabank_\1*metabankSize)+end
;   .print test," ",test2,"\n"
  
  .if ((start < metabank_\1_loadAddr)||(end >= (metabank_\1_loadAddr+metabank_\1_size)))
    .print "Bad metabank unbackground: ",name," ",start," ",end,"\n"
    .fail
  .endif
  
  .unbackground ((metabank_\1*metabankSize)+start) ((metabank_\1*metabankSize)+end)
.endm

.macro unbackgroundMetabankByRawOffset ARGS name, start, end
  unbackgroundMetabank name, metabank_\1_loadAddr+start, metabank_\1_loadAddr+end
.endm

; better solution would probably be to divide by a large number and use ceil()...
; oh wait, now that i'm looking at the function list again,
; i see something even easier!
;.function getBit(val,shift) ((val&(1<<shift))>>shift)
;.function oneIfNonzero(val) getBit(val,0)|getBit(val,1)|getBit(val,2)|getBit(val,3)|getBit(val,4)|getBit(val,5)|getBit(val,6)|getBit(val,7)
.function oneIfNonzero(val) abs(sign(val))
.function zeroIfNonzero(val) abs( (oneIfNonzero(val)-1) )

;============================
; sectioning
;============================

.macro makeSectionWithBanks ARGS name,specifiers,bankStart,bankEnd
;  .print content,"\n"
  .if (NARGS == 2)
    .section "\1" \2
  .elif (NARGS == 3)
    .section "\1" \2 BANKS \3
  .else
    .section "\1" \2 BANKS \3-\4
  .endif
.endm

.macro makeSemiSuperfreeSection ARGS name,bankStart,bankEnd
;  .print content,"\n"
  .if NARGS == 2
    makeSectionWithBanks name,semisuperfree,bankStart
  .else
    makeSectionWithBanks name,semisuperfree,bankStart,bankEnd
  .endif
.endm

.macro makeSectionInFreeBank ARGS name
  makeSemiSuperfreeSection name,metabank_freebank_0,metabank_freebank_3
.endm

.macro makeSectionInCodeBank ARGS name
  makeSemiSuperfreeSection name,metabank_codebank_0,metabank_codebank_3
.endm

.macro makeSectionInGrpBank ARGS name
  makeSemiSuperfreeSection name,metabank_grpbank_0,metabank_grpbank_3
.endm

;============================
; scripts
;============================

.macro text_jumpToPossiblyNewLoc ARGS target
  ; HACK: we need to do a DB that resolves to 0x18 (standard jump) if the target
  ; is in the original area, and 0x17 (new area jump) if it is in the new area.
  ; we can't use IF for this because that needs immediate data, so we have to
  ; make it happen within a formula.
  ; 
  ; metabank_newtext-(:target) resolves to 0 if in maintext, 1 otherwise.
  ; (HACK: requires metabank_maintext==0, metabank_newtext==1.)
  ; so, subtract that from 0x18 to get the correct opcode.
;  .db $18-((:target)-metabank_maintext)
  
;  .db $18-(oneIfNonzero((:target)-metabank_maintext))
  
  ; (:target)-metabank_maintext) is nonzero if target is not in maintext.
  ; only one of the two top-level terms will resolve to an actual op,
  ; and the other will be zero.
  .db (textop_jumpNewText*(oneIfNonzero((:target)-metabank_maintext)))+(textop_jump*(zeroIfNonzero((:target)-metabank_maintext)))
    .dw target
.endm

;==============================================================================
; unbackgrounds
;==============================================================================

; free space in expanded banks
; .unbackground $60000 $7FFFF

unbackgroundMetabank "bank00", $FEB0, $FFDF
  ; disused scrap of drawTextChar
  unbackgroundMetabank "bank00",$E675,$E680
unbackgroundMetabank "bank01", $DF70, $DFFF
  ; old backup memory strings (overwritten)
  unbackgroundMetabank "bank01", $D740, $D79D
unbackgroundMetabank "race", $5C00, $5FFF
;unbackgroundMetabank "maintext", $BE70, $BFFF
unbackgroundMetabank "maintext", $4000, $BFFF
unbackgroundMetabank "newtext", $4000, $BFFF
unbackgroundMetabank "gamescript", $BBC0, $BFFF

unbackgroundMetabankByRawOffset "freebank_0", $0000, $1FFF
unbackgroundMetabankByRawOffset "freebank_1", $0000, $1FFF
unbackgroundMetabankByRawOffset "freebank_2", $0000, $1FFF
unbackgroundMetabankByRawOffset "freebank_3", $0000, $1FFF

unbackgroundMetabankByRawOffset "codebank_0", $0000, $1FFF
unbackgroundMetabankByRawOffset "codebank_1", $0000, $1FFF
unbackgroundMetabankByRawOffset "codebank_2", $0000, $1FFF
unbackgroundMetabankByRawOffset "codebank_3", $0000, $1FFF

unbackgroundMetabankByRawOffset "grpbank_0", $0000, $3FFF
unbackgroundMetabankByRawOffset "grpbank_1", $0000, $3FFF
unbackgroundMetabankByRawOffset "grpbank_2", $0000, $3FFF
unbackgroundMetabankByRawOffset "grpbank_3", $0000, $3FFF

;==============================================================================
; auto-generated files
;==============================================================================

.include "gen/maintext_data.inc"
.include "gen/maintext_overwrite.inc"
.include "gen/charmap.inc"

;==============================================================================
; defines
;==============================================================================

;============================
; banking
;============================

.define newTextBaseBank $40
.define freeBankBaseBank $44
.define codeBankBaseBank $48
.define grpBankBaseBank $4C

;============================
; textscript
;============================

.define numWindowSlots 8

; if set, window's scripts read text from the new area
.define newWindowFlags_inNewTextArea           (1<<0)
; if set, newly composited material goes to back buffer.
; otherwise, front.
.define newWindowFlags_targetingBackCompBuffer (1<<1)
; set if kerning has resulted in backing up into the previous pattern
.define newWindowFlags_backedUpPrinting        (1<<2)

; flags that are saved to/restored from the stack during a subtext call
.define savedNewWindowFlags (newWindowFlags_inNewTextArea)

;============================
; printing
;============================

;=====
; font data
;=====

; base index of font data.
; subtract this from the raw character ID to get internal font index num.
.define fontBaseIndex $20

; characters within this range are printed through the normal font system.
; those outside the range are handled as in the original game
; (written to the tilemap using preloaded symbols).
.define fontStdStartIndex $21
.define fontStdEndIndex $80

.define fontCharRawW 8
.define fontCharRawH 8
.define bytesPerRawFontChar ((fontCharRawW / 8) * fontCharRawH)

.define fontCompBufferW 8
.define fontCompBufferH 8
.define fontCompBufferSize ((fontCompBufferW / 8) * fontCompBufferH)

; minimum length of an encoded run in the kerning data
.define kerning_runLenBase 3

.define numRegisteredFonts 0

.macro reverseRegisterFont ARGS num, name
  .define registeredFontName_\1 \2
.endm

.macro registerFont ARGS name
  ; symbol fontId_[name] contains font's ID
  .redefine fontId_\1 numRegisteredFonts
  ; symbol registeredFontName_[name] maps ID to name
  reverseRegisterFont numRegisteredFonts, \1
  
  .redefine numRegisteredFonts numRegisteredFonts+1
.endm

registerFont "std"
registerFont "monospace"
registerFont "monospace_it"
registerFont "italic"
registerFont "bold"
registerFont "asciistrict"

;=====
; print tile mapping setup
;=====

.macro defineWithNameSuffix ARGS name,suffix,value
  .define \1_\2 \3
.endm

; the maximum possible number of tiles we would ever even consider handling.
; not the actual number of tiles -- this is only used by the assembler as a
; bound on loops that check which tiles have been assigned.
.define maxPossiblePrintTileMappings 512
; limit of tile numbers generated by the reverse mapping process
.define maxPossiblePrintTileReverseMappings 512

; actual number of tile slots in use.
; redefined to its final value after the assignments below.
.define numPrintTileSlots 0
.macro definePrintTileMappingEntry ARGS tileNum
;  .define printTileMapping_\1 1

  .redefine definePrintTileMappingEntry_index0 (numPrintTileSlots+0)
  .redefine definePrintTileMappingEntry_index1 (numPrintTileSlots+1)
  .redefine definePrintTileMappingEntry_tile0 (tileNum+0)
  ; the palette flag goes in the low bit of the high byte rather than the
  ; actual palette byte.
  ; this is to simplify access to the reverse mapping table.
  .redefine definePrintTileMappingEntry_tile1 ((tileNum+0)|$0100)
  
;  .print "here: ",definePrintTileMappingEntry_index0," ",definePrintTileMappingEntry_tile0,"\n"
  
  ; normal mapping (index -> tile id)
  ; front palette
  defineWithNameSuffix printTileMapping,definePrintTileMappingEntry_index0,definePrintTileMappingEntry_tile0
  ; back palette
  defineWithNameSuffix printTileMapping,definePrintTileMappingEntry_index1,definePrintTileMappingEntry_tile1
  
  ; reverse mapping (tile id -> index)
  ; front palette
  defineWithNameSuffix printTileReverseMapping,definePrintTileMappingEntry_tile0,definePrintTileMappingEntry_index0
  ; back palette
  defineWithNameSuffix printTileReverseMapping,definePrintTileMappingEntry_tile1,definePrintTileMappingEntry_index1
  
  ; each definition counts twice (once for front palette and once for back)
  .redefine numPrintTileSlots numPrintTileSlots+2
.endm

;=====
; define print tile entries
;=====

; .rept $3F INDEX count
;   definePrintTileMappingEntry $40+count
; .endr
; 
; .rept $40 INDEX count
;   definePrintTileMappingEntry $A1+count
; .endr

.rept $100 INDEX count
  .if ((count >= $40) && (count <= $7E))
    definePrintTileMappingEntry count
  .elif ((count >= $A1) && (count <= $DF))
    ; exclude dot character, used on password screen, etc.
    .if (count == $A5)
    
    .else
      definePrintTileMappingEntry count
    .endif
  .endif
.endr

;=====
; derived variables of above
;=====

; number of bytes needed for bitflag array tracking allocated tiles
.define printTileMappingAllocArraySize floor((numPrintTileSlots+7)/8)

;==============================================================================
; memory
;==============================================================================

;============================
; existing
;============================

;=====
; windowing and textscripts
;=====

.define zp_buttonsPressed $20
.define zp_buttonsPressedLastFrame $25
.define zp_buttonsTriggered $2A

.define zp_textscriptPtr $5E
.define zp_allTextScriptsCombinedState $62

.define textscrState_array $222D
.define textPtrLo_array $2235
.define textPtrHi_array $223D

.define windowContentRestoreStackPosLo_array $2245
.define windowContentRestoreStackPosHi_array $224D
.define windowY_array $2255
.define windowX_array $225D
.define windowH_array $2265
.define windowW_array $226D
.define windowPrintY_array $2275
.define windowPrintX_array $227D
.define windowPrintTilemapHiByte_array $2285
.define windowPrintSpeed_array $228D
.define windowPrintCharDelay_array $2295
.define windowPrintDelay_array $229D
.define textscrCallStackPos_array $22BD
.define textscrCallStack_array $22C5

.define currentSpriteCount $2371

;============================
; new
;============================

; this word is apparently reserved for use by fetch2bGamescrValToB6 ($FE95),
; which doesn't appear to actually be called anywhere, so we should be fine
; using it for anything we want
.define zp_scratchWord0 $B6
  .define zp_scratchWord0_lo $B6
  .define zp_scratchWord0_hi $B7
; FIXME: this may not be safe.
; haven't seen this written anywhere, and there's no obvious code that does so,
; but it may happen somewhere.
; need to check.
.define zp_scratchWord1 $FE
  .define zp_scratchWord1_lo $FE
  .define zp_scratchWord1_hi $FF
; well, actually, since the last bit of zero page memory is apparently never
; used for anything, i guess we might as well do something with it!
; these are used for code that doesn't run out of the vsync interrupt.
; the alternative would be to have the vsync interrupt save and restore the
; above registers instead, but if we've got the free memory, no reason not to
; make use of it.
.define zp_nonInt_scratchWord0 $FA
  .define zp_nonInt_scratchWord0_lo $FA
  .define zp_nonInt_scratchWord0_hi $FB
.define zp_nonInt_scratchWord1 $FC
  .define zp_nonInt_scratchWord1_lo $FC
  .define zp_nonInt_scratchWord1_hi $FD

.define newMainMemory_base $3E80

; max size of self-modifying routines that are uploaded to ram.
; we can't put the computed directly in the enum below, so we use predefined
; values here; these are later asserted against the actual size of the generated
; code to ensure there's no overflow.
.define printShifter_right_maxSize 31
.define printShifter_left_maxSize 35

.enum newMainMemory_base
  mainMem_scratchWord0 .dw
    mainMem_scratchWord0_lo db
    mainMem_scratchWord0_hi db
  mainMem_scratchWord1 .dw
    mainMem_scratchWord1_lo db
    mainMem_scratchWord1_hi db
  mainMem_scratchWord2 .dw
    mainMem_scratchWord2_lo db
    mainMem_scratchWord2_hi db
  
  mainMem_nonInt_scratchWord0 .dw
    mainMem_nonInt_scratchWord0_lo db
    mainMem_nonInt_scratchWord0_hi db
  mainMem_nonInt_scratchWord1 .dw
    mainMem_nonInt_scratchWord1_lo db
    mainMem_nonInt_scratchWord1_hi db
  mainMem_nonInt_scratchWord2 .dw
    mainMem_nonInt_scratchWord2_lo db
    mainMem_nonInt_scratchWord2_hi db
  
  ;=====
  ; print tile allocation
  ;=====
  
  ; index of most recently allocated print tile, so we can do a circular search
  ; rather than going through the whole index to find the first open position
  lastAllocatedPrintIndex dw
  
  ; bitflag array of print tiles currently in use
  printTileAlloc_array dsb printTileMappingAllocArraySize
  
  ; set to a nonzero value to disable automatic tile deallocation.
  ; this is necessary in some scenes that make use of line interrupts,
  ; as line interrupts are inhibited during processing of the game's main logic,
  ; so if it runs too long, the line interrupts will be missed and cause
  ; graphical glitches.
  ; when enabled, extra steps like VRAM readbacks will be entirely skipped to
  ; avoid slowing down the game any more than necessary.
  printing_inhibitAutoTileDealloc db
  
  ;=====
  ; printing
  ;=====
  
  ; temporary buffer for shifted-out content when printing
  printing_shiftOutBuffer dsb fontCompBufferSize
  
  ;=====
  ; window arrays
  ;
  ; each entry here is duplicated for each of the 8 window slots
  ;=====
  
  ; additional flags for windows (target text area, etc.)
  newWindowFlags_array dsb numWindowSlots
  ; current position in textscrNewTextAreaFlagStack_array for each slot.
  ; this is initialized to (slotNum*2) when window is opened.
  textscrNewTextAreaFlagStackPos_array dsb numWindowSlots
  ; stack to store new text area flags for subtext calls.
  ; max recursion depth of calls is 2, so 2 bytes per window slot.
  textscrNewTextAreaFlagStack_array dsb numWindowSlots*2
  
  ; front composition buffers for each window slot
  printCompFrontBuffer_array dsb numWindowSlots*fontCompBufferSize
  ; back composition buffers for each window slot
  printCompBackBuffer_array dsb numWindowSlots*fontCompBufferSize
  
  ; front target tile index arrays for each window slow
  printTargetIndexFrontLo_array dsb numWindowSlots
  printTargetIndexFrontHi_array dsb numWindowSlots
  ; back target tile index arrays for each window slow
  printTargetIndexBackLo_array dsb numWindowSlots
  printTargetIndexBackHi_array dsb numWindowSlots
  
  ; current fine x-pos of printing
  printing_fineX_array dsb numWindowSlots
  
  ; currently selected font id
  ; WARNING: this is NOT reset when a new script is started!
  ; it "sticks". this is to facilitate gamescripts which need to change the
  ; font (e.g. to monospace for the name entry and password screens).
  ; make sure textscripts that change the font always restore it to the
  ; expected default.
  printing_currentFont_array dsb numWindowSlots
  
  ; id of most recently printed normal char
  printing_prevChar_array dsb numWindowSlots
  
  printing_windowRestoreSubState_array dsb numWindowSlots
  
  ; ROM_printShifter_right is copied here
  RAM_printShifter_right dsb printShifter_right_maxSize
  
  ; ROM_printShifter_left is copied here
  RAM_printShifter_left dsb printShifter_left_maxSize
  
  activeTextscriptCount db
  
;   getNextWordWidth_step_recursionLevel db
  
  newMainMemory_end  .db
.ende

.if (newMainMemory_end > $4000)
  .fail "Main memory overruns available space\n"
.endif

.define newMainMemory_size newMainMemory_end-newMainMemory_base

;==============================================================================
; routines
;==============================================================================

;============================
; existing
;============================

.define waitForVsync $E17D

.define runSingleTextscrIteration $E33B

.define setMARRWithX $E8D0
.define setMAWRWithX $E8D4

.define loadHalfTilemapList $EB0B
.define loadFullTilemapList $EBA4
.define loadGraphicList $ECFC

; 1F/1E = target y/x
; returns $7A-7B = corresponding tilemap addr
.define tilemapPosToAddr $EE00

.define generateSatEntry $EE7E
.define transferSatEntries $EE51

; $8F/$90 = operands
; $91-92 = result
.define mult8 $F23B
  .define zp_mult8_in0 $8F
  .define zp_mult8_in1 $90
  .define zp_mult8_out0 $91
  .define zp_mult8_out1 $92

;==============================================================================
; macros
;==============================================================================

;==================================
; trampoline
;==================================

.macro trampolineToMpr6 ARGS bank, ptr
  jsr trampolineCallMpr6
;     .db codeBankBaseBank+(metabankNum-metabank_codebank_0)
;     .dw ptr
    .db >(ptr-1)
    .db <(ptr-1)
    .db bank
.endm

.macro trampolineToCodeBank ARGS metabankNum, ptr
  trampolineToMpr6 (codeBankBaseBank+(metabankNum-metabank_codebank_0)), ptr
.endm

.macro trampolineToCodeBankRoutine ARGS ptr
  trampolineToCodeBank :ptr, ptr
.endm

.macro nonInt_trampolineToMpr6 ARGS bank, ptr
  jsr trampolineCall_nonInt
    .db >(ptr-1)
    .db <(ptr-1)
    .db bank
.endm

.macro nonInt_trampolineToCodeBank ARGS metabankNum, ptr
  nonInt_trampolineToMpr6 (codeBankBaseBank+(metabankNum-metabank_codebank_0)), ptr
.endm

.macro nonInt_trampolineToCodeBankRoutine ARGS ptr
  nonInt_trampolineToCodeBank :ptr, ptr
.endm

;============================
; misc
;============================

; ; note: uses 15 bytes
; .macro TRAMPOLINE_CALL args label
;   lda #<label
;   sta scratchSpace1.w
;   lda #>label
;   sta scratchSpace2.w
;   lda #:label
;   jsr doTrampolineCall
; .endm
; 
; ; ; note: uses 14 bytes
; ; .macro TRAMPOLINE_SIMPLE args label
; ;   tma #$40
; ;   pha
; ;   lda #:label
; ;   tam #$40
; ; ;     lda #>trampolineSimpleRet_\2-1
; ; ;     pha
; ; ;     lda #<trampolineSimpleRet_\2-1
; ; ;     pha
; ; ;     
; ; ;     cla
; ; ;     jsr useFollowingJumpTable
; ; ;     .dw label
; ;     jsr label
; ; ;   trampolineSimpleRet_\2:
; ;   pla
; ;   tam #$40
; ;   rts
; ; .endm
; 
; ; slow byte-by-byte VRAM copy,
; ; written on the assumption that speed is less important
; ; than memory for the limited use cases here.
; ; dst is in words (i.e. byteAddr/2).
; ; size must be divisible by 2.
; .macro COPY_TO_VRAM args src, dst, size
;   ; target reg = mawr
; ;  st0 #vdp_reg_mawr
;   lda #vdp_reg_mawr
;   sta zpVideoReg.b
;   sta vdp_select.w
;   ; set write address
;   st1 #<dst
;   st2 #>dst
;   
;   ; target reg = vram write
; ;  st0 #vdp_reg_vwr
;   lda #vdp_reg_vwr
;   sta zpVideoReg.b
;   sta vdp_select.w
;   
;   lda zpScratch+0.b
;   pha
;   lda zpScratch+1.b
;   pha
;     lda #<src
;     sta zpScratch+0.b
;     lda #>src
;     sta zpScratch+1.b
;   ;  lda #<(size/2)
;   ;  sta scratchWord1+0.w
;   ;  lda #>(size/2)
;   ;  sta scratchWord1+1.w
;     -:
;       ; copy first byte
;       lda (zpScratch)
;       sta vdp_data+0.w
;       ; increment src
;       inc zpScratch+0.b
;       bne +
;         inc zpScratch+1.b
;       +:
;       
;       ; copy second byte
;       lda (zpScratch)
;       sta vdp_data+1.w
;       ; increment src
;       inc zpScratch+0.b
;       bne +
;         inc zpScratch+1.b
;       +:
;       
;       ; loop while srcpos != endpos
;       lda zpScratch+0.b
;       cmp #<(src+size)
;       bne -
;       lda zpScratch+1.b
;       cmp #>(src+size)
;       bne -
;   pla
;   sta zpScratch+1.b
;   pla
;   sta zpScratch+0.b
;     
; .endm
; 
; ; .macro SAVE_MPR2
; ;   tma #$04
; ;   pha
; ; .endm
; ; 
; ; .macro RESTORE_MPR2
; ;   pla
; ;   tam #$04
; ; .endm

;==============================================================================
; new global code
;==============================================================================

;=======================================
; trampoline
;=======================================
  
; .bank $00 slot 7
; .section "trampoline routine 1" free
;   ;=====
;   ; caller sets scratchSpace1/2 to target address.
;   ; A = target bank for MPR6 during call.
;   ;=====
;   
;   doTrampolineCall:
;     ; temporarily save target bank
;     sta scratchSpace0.w
;     ; save MPR6
;     tma #$40
;     pha
;     ; set new bank
;     lda scratchSpace0.w
;     tam #$40
;       ; set up jump
;       ; JMP opcode
;   ;    lda #$20
;       lda #$4C
;       sta scratchSpace0.w
;       jsr scratchSpace0
;     ; save return value from call
;     sta scratchSpace0.w
;     ; restore MPR6
;     pla
;     tam #$40
;     ; return A
;     lda scratchSpace0.w
;     rts
; .ends

;==============================================================================
; DEBUG
;==============================================================================

.if DEBUG_BOOT_TO_CREDITS != 0
;   .define creditsScriptAddr $7825
;   .bank metabank_bank00
;   .section "debug: boot to credits 1" orga $E0E6 overwrite
;     ; 78fe
;     lda #<creditsScriptAddr
;     sta $00.b
;     lda #>creditsScriptAddr
;     sta $01.b
;   .ends
  
  ; this is a bit imperfect (startup logo fades in then immediately vanishes
  ; as we jump to credits), but is probably good enough
  .define creditsScriptAddr $758C
  .bank metabank_gamescript
;   .section "debug: boot to credits 1" orga $451E overwrite
  .section "debug: boot to credits 1" orga $451B overwrite
    gamescript_jump creditsScriptAddr
  .ends
.endif

;==============================================================================
; bank 00
;==============================================================================

;==================================
; trampoline
;==================================

.bank metabank_bank00
.section "trampoline 2" free
  ; call should be immediately followed by:
  ; - 2b address to call, in rts-ready format (big-endian and minus one)
  ; - 1b target bank
  trampolineCallMpr6:
    ; preserve regs
    php
      sta mainMem_scratchWord0_lo.w
      sty mainMem_scratchWord0_hi.w
    pla
    sta mainMem_scratchWord1_hi.w
    
    ; get return addr
    pla
    sta zp_scratchWord0_lo.b
    pla
    sta zp_scratchWord0_hi.b
    
    ; set up this call's new return addr
    lda #$03
    clc
    adc zp_scratchWord0_lo.b
    sta mainMem_scratchWord1_lo.b
    cla
    adc zp_scratchWord0_hi.b
    pha
    lda mainMem_scratchWord1_lo.b
    pha
    
    ; save current bank
    tma #$40
    pha
;       ; switch to target bank
;       ldy #1
;       lda (zp_scratchWord0.b),Y
;       tam #$40
;       
;       ; fetch target addr
;       iny
;       lda (zp_scratchWord0.b),Y
;       pha
;         iny
;         lda (zp_scratchWord0.b),Y
;         sta zp_scratchWord0_hi.b
;       pla
;       sta zp_scratchWord0_lo.b
;       
;       ; use preserved A/Y values
;       lda mainMem_scratchWord0_lo.w
;       ldy mainMem_scratchWord0_hi.w
;       
;       ; make call
;       jsr (zp_scratchWord0.b)
      
      ; push local return target
      lda #>(@returnTarget-1)
      pha
      lda #<(@returnTarget-1)
      pha
      
      ; fetch call target
      ldy #1
      lda (zp_scratchWord0.b),Y
      pha
      iny
      lda (zp_scratchWord0.b),Y
      pha
      
      ; switch to target bank
      iny
      lda (zp_scratchWord0.b),Y
      tam #$40
      
      ; use preserved register values
      lda mainMem_scratchWord1_hi.w
      pha
        lda mainMem_scratchWord0_lo.w
        ldy mainMem_scratchWord0_hi.w
      plp
      
      ; make call
      rts
      
      ; preserve returned values from call
      @returnTarget:
      php
        sta mainMem_scratchWord1_lo.b
      pla
      sta mainMem_scratchWord1_hi.b
    ; restore original bank
    pla
    tam #$40
    
    ; return values from call
    lda mainMem_scratchWord1_hi.b
    pha
      lda mainMem_scratchWord1_lo.b
    plp
    rts
  
  ; alternate trampoline routine for code that runs outside interrupts.
  ; same structure, but uses different memory to avoid getting clobbered if
  ; an interrupt routine uses a trampoline.
  trampolineCall_nonInt:
    ; preserve regs
    php
      sta mainMem_nonInt_scratchWord0_lo.w
      sty mainMem_nonInt_scratchWord0_hi.w
    pla
    sta mainMem_nonInt_scratchWord1_hi.w
    
    ; get return addr
    pla
    sta zp_nonInt_scratchWord0_lo.b
    pla
    sta zp_nonInt_scratchWord0_hi.b
    
    ; set up this call's new return addr
    lda #$03
    clc
    adc zp_nonInt_scratchWord0_lo.b
    sta mainMem_nonInt_scratchWord1_lo.b
    cla
    adc zp_nonInt_scratchWord0_hi.b
    pha
    lda mainMem_nonInt_scratchWord1_lo.b
    pha
    
    ; save current bank
    tma #$40
    pha
      
      ; push local return target
      lda #>(@returnTarget-1)
      pha
      lda #<(@returnTarget-1)
      pha
      
      ; fetch call target
      ldy #1
      lda (zp_nonInt_scratchWord0.b),Y
      pha
      iny
      lda (zp_nonInt_scratchWord0.b),Y
      pha
      
      ; switch to target bank
      iny
      lda (zp_nonInt_scratchWord0.b),Y
      tam #$40
      
      ; use preserved register values
      lda mainMem_nonInt_scratchWord1_hi.w
      pha
        lda mainMem_nonInt_scratchWord0_lo.w
        ldy mainMem_nonInt_scratchWord0_hi.w
      plp
      
      ; make call
      rts
      
      ; preserve returned values from call
      @returnTarget:
      php
        sta mainMem_nonInt_scratchWord1_lo.b
      pla
      sta mainMem_nonInt_scratchWord1_hi.b
    ; restore original bank
    pla
    tam #$40
    
    ; return values from call
    lda mainMem_nonInt_scratchWord1_hi.b
    pha
      lda mainMem_nonInt_scratchWord1_lo.b
    plp
    rts
.ends

;==================================
; initialize new memory on reset
;==================================

; .bank metabank_bank00
; .section "init new memory on reset 1" orga $E148 overwrite
;   jsr newMemoryInit
;   nop
; .ends
; 
; .bank metabank_bank00
; .section "init new memory on reset 2" free
;   newMemoryInit:
;     stz newMainMemory_base.w
;     tii newMainMemory_base,newMainMemory_base+1,newMainMemory_size-1
;     ; make up work
;     lda #$E7
;     sta $46.b
;     rts
; .ends

.bank metabank_bank00
.section "init new memory on reset 1" orga $E14A size 6 overwrite
  nonInt_trampolineToCodeBankRoutine newMemoryInit
.ends

makeSectionInCodeBank "init new memory on reset 2"
  newMemoryInit:
    ; make up work
    sta $46.b
    sta $48.b
    sta $4A.b
    
    ; init new ram
    stz newMainMemory_base.w
    tii newMainMemory_base,newMainMemory_base+1,newMainMemory_size-1
    
    ; upload RAM routines
    tii ROM_printShifter_right,RAM_printShifter_right,(ROM_printShifter_right_end-ROM_printShifter_right)
    tii ROM_printShifter_left,RAM_printShifter_left,(ROM_printShifter_left_end-ROM_printShifter_left)
    
    rts
  
;   ROM_printShifter_right:
;     bra +
;     +:
;     .rept 4
;       lsr
;       ror printing_shiftOutBuffer.w,X
;     .endr
;     rts
;   ROM_printShifter_right_end:
  
  ROM_printShifter_right:
    -:
      lda (zp_scratchWord0.b),Y
      
      @branchInstr:
      bra +
      +:
      .rept 4
        lsr
        ror printing_shiftOutBuffer.w,X
      .endr
      
      ora (zp_scratchWord1.b),Y
      sta (zp_scratchWord1.b),Y
      
      inx
      iny
      cpy #fontCompBufferSize
      bne -
    rts
  ROM_printShifter_right_end:
  
  .if (ROM_printShifter_right_end-ROM_printShifter_right) > printShifter_right_maxSize
    .print "ROM_printShifter_right is too large (",(ROM_printShifter_right_end-ROM_printShifter_right)," bytes, max ",printShifter_right_maxSize,")\n"
    .fail
  .endif
  
  ROM_printShifter_left:
    -:
      stz RAM_printShifter_left+(ROM_printShifter_left@finalLoadInstr-ROM_printShifter_left)+1.w
      lda (zp_scratchWord0.b),Y
      
      @branchInstr:
      bra +
      +:
      .rept 3
        asl
        rol RAM_printShifter_left+(ROM_printShifter_left@finalLoadInstr-ROM_printShifter_left)+1.w
      .endr
      
      sta printing_shiftOutBuffer.w,X
      
      @finalLoadInstr:
      lda #$00
      ora (zp_scratchWord1.b),Y
      sta (zp_scratchWord1.b),Y
      
      inx
      iny
      cpy #fontCompBufferSize
      bne -
    rts
  ROM_printShifter_left_end:
  
  .if (ROM_printShifter_left_end-ROM_printShifter_left) > printShifter_left_maxSize
    .print "ROM_printShifter_left is too large (",(ROM_printShifter_left_end-ROM_printShifter_left)," bytes, max ",printShifter_left_maxSize,")\n"
    .fail
  .endif
.ends

; .bank metabank_bank00
; .section "trampoline test 1" orga $E152 size 6 overwrite
;   trampolineToCodeBank :trampolineTest,trampolineTest
; .ends
; 
; makeSectionInCodeBank "trampoline test 2"
;   trampolineTest:
;     ; make up work
;     sta $47.b
;     sta $49.b
;     sta $4B.b
;     rts
; .ends

;testmacro metabank_bank00,metabank_bank01
;makeSectionWithBanks "test 1",SEMISUPERFREE,metabank_maintext,metabank_newtext
; makeSemiSuperfreeSection "test 1",metabank_maintext,metabank_newtext
;   test:
;     .db $00
; .ends

; makeSectionInFreeBank "test 1"
;   test:
;     .db $00
; .ends

;==============================================================================
; textscripts
;==============================================================================

;makeSectionInCodeBank "new textscript area"
; put this in a fixed bank because we otherwise have no way to use FORCE
; to put data at a specific location.
; we need this to make the "censor" script appear at its original position,
; so textscripts can access it despite the old bank01 being paged out.
makeSemiSuperfreeSection "new textscript area",metabank_codebank_0
  ; main section for all common textscript resources.
  ; append anything that needs access to common handling routines
  ; to this.
.ends

;==================================
; new window init
;==================================

.bank metabank_bank00
.section "extra window init 1" orga $E6FD size 6 overwrite
  nonInt_trampolineToCodeBankRoutine extraWindowInit
.ends

.section "extra window init 2" APPENDTO "new textscript area"
  extraWindowInit:
    ; make up work
    txa
    asl
    asl
    sta textscrCallStackPos_array.w,X
    
    ; init new ram values
    
    ; all flags to default
    stz newWindowFlags_array.w,X
    ; initial text area flag stack pos
    txa
    asl
    sta textscrNewTextAreaFlagStackPos_array.w,X
    ; TODO: more?
    
    rts
.ends

;==================================
; new textscript init
;==================================

.bank metabank_bank00
.section "extra textscript init 1" orga $E747 size 6 overwrite
  nonInt_trampolineToCodeBankRoutine extraTextscriptInit
.ends

.section "extra textscript init 2" APPENDTO "new textscript area"
  extraTextscriptInit:
    ; make up work
    sta textPtrHi_array.w,X
    
    ; clear text area flag (all textscripts should start in the
    ; original text area)
    lda newWindowFlags_array.w,X
    and #~newWindowFlags_inNewTextArea
    sta newWindowFlags_array.w,X
    
    ; make up work
    lda textscrState_array.w,X
    rts
.ends

;==================================
; new textscript op handling
;==================================

.macro overwriteTextOpHandler ARGS opNum,ptr
  .bank metabank_bank00
  .section "textscr op overwrite \1 \@ 1" orga $E90C+opNum overwrite
    .db <ptr
  .ends
  
  .bank metabank_bank00
  .section "textscr op overwrite \1 \@ 2" orga $E92C+opNum overwrite
    .db >ptr
  .ends
.endm

;=====
; jumpnew
;=====

.bank metabank_bank00
.section "new textscript ops: jumpnew 1" free
  textscript_jumpNew_handler:
    trampolineToCodeBankRoutine textscript_jumpNew_handler_sub
    rts
.ends

.section "new textscript ops: jumpnew 2" APPENDTO "new textscript area"
  textscript_jumpNew_handler_sub:
    ; set new text area flag
    lda newWindowFlags_array.w,X
    ora #newWindowFlags_inNewTextArea
    sta newWindowFlags_array.w,X
    
    jsr loadNewScriptPtr
    
    ; switch to new banks
    jmp loadNewTextAreaBanks
  
  loadNewScriptPtr:
    ; read and set new script pointer
    lda (zp_textscriptPtr.b),Y
    pha
      iny
      lda (zp_textscriptPtr.b),Y
      sta zp_textscriptPtr+1.b
    pla
    sta zp_textscriptPtr+0.b
    
    cly
    rts
.ends

overwriteTextOpHandler textop_jumpNewText,textscript_jumpNew_handler

;=====
; setfont
;=====

.bank metabank_bank00
.section "new textscript ops: setfont 1" free
  textscript_setFont_handler:
    trampolineToCodeBankRoutine textscript_setFont_handler_sub
    rts
.ends

.section "new textscript ops: setfont 2" APPENDTO "new textscript area"
  textscript_setFont_handler_sub:
    ; fetch param (font id)
    lda (zp_textscriptPtr.b),Y
    iny
    ; set as current font
    sta printing_currentFont_array.w,X
    rts
.ends

overwriteTextOpHandler textop_setFont,textscript_setFont_handler

;==================================
; existing op modifications
;==================================

;=====
; newline
;=====

; .bank metabank_bank00
; .section "new textscript newline logic 1" orga $E5AB size 6 overwrite
;   trampolineToCodeBankRoutine doNewTextscriptNewline
;   ; !!! drop through into carriage return handler !!!
; .ends

.section "new textscript newline logic 2" APPENDTO "new textscript area"
  doNewTextscriptNewline:
    ; reset window's printing state for upcoming line
    jsr resetPrintingForNewLine
    
    ; make up work
    inc windowPrintY_array.w,X
    inc windowPrintY_array.w,X
    rts
.ends

;=====
; carriage return
;=====

.bank metabank_bank00
.section "new textscript carriage return logic 1" orga $E5B3 overwrite
  jmp doNewCarriageReturn
.ends

.bank metabank_bank00
.section "new textscript carriage return logic 2" free
;   doNewCarriageReturn:
;     trampolineToCodeBankRoutine doNewCarriageReturn_sub
;     rts
  doNewCarriageReturn:
    ; make up work
    sta windowPrintX_array.w,X
    ; reset
    jmp resetPrintingForNewLine
.ends

; .section "new textscript carriage return logic 3" APPENDTO "new textscript area"
;   doNewCarriageReturn_sub:
;     ; make up work
;     sta windowPrintX_array.w,X
;     
;     ; reset window's printing state for upcoming line
;     jmp resetPrintingForNewLine
; .ends

;=====
; jump
;=====

.bank metabank_bank00
.section "new textscript jump logic 1" orga $E48C size 7 overwrite
;  trampolineToCodeBankRoutine doNewTextscriptJump
;  rts
  jmp doNewTextscriptJump
.ends

.section "new textscript jump logic 2" APPENDTO "new textscript area"
  doNewTextscriptJump:
    ; clear new text area flag
    lda newWindowFlags_array.w,X
    and #~newWindowFlags_inNewTextArea
    sta newWindowFlags_array.w,X
    
    ; make up work (load new handler)
    jsr loadNewScriptPtr
    
    ; load orig text banks
    jmp loadOrigTextAreaBanks
.ends

;=====
; call
;=====

.bank metabank_bank00
.section "new textscript call logic 1" orga $E472 overwrite
;  trampolineToCodeBankRoutine doNewTextscriptCall
;;  jmp $E48C
;  rts
  jmp doNewTextscriptCall
.ends

.section "new textscript call logic 2" APPENDTO "new textscript area"
  doNewTextscriptCall:
    ; make up work
    ; put current script pos on call stack
    phx
      lda textscrCallStackPos_array.w,X
      tax
      
      tya
      clc
      adc zp_textscriptPtr+0.b
      sta textscrCallStack_array.w,X
      inx
      lda zp_textscriptPtr+1.b
      adc #$00
      sta textscrCallStack_array.w,X
      inx
      txa
    plx
    sta textscrCallStackPos_array.w,X
    
    ; repeat above process for text area flag stack
    phy
    phx
      ; save text area selection flag to Y
      lda newWindowFlags_array.w,X
      and #savedNewWindowFlags
      tay
      
      ; get target position in stack
      lda textscrNewTextAreaFlagStackPos_array.w,X
      tax
      
      ; save flag to stack
      tya
      sta textscrNewTextAreaFlagStack_array.w,X
      
      inx
      txa
    plx
    ply
    ; save updated stack pos
    sta textscrNewTextAreaFlagStackPos_array.w,X
    
    ; we don't have to do any of the text area reset work;
    ; the jump handler that this routine ends up dropping into handles that
    
    ; clear new text area flag
;     lda newWindowFlags_array.w,X
;     and #~newWindowFlags_inNewTextArea
;     sta newWindowFlags_array.w,X
;     
;     ; load orig text banks
;     jsr loadOrigTextAreaBanks
    
;    rts
    
    ; make things a little faster and just call the jump subroutine ourselves
    jmp doNewTextscriptJump
.ends

;=====
; terminated call
;=====

.bank metabank_bank00
.section "new textscript terminator logic 1" orga $E540 overwrite
;  trampolineToCodeBankRoutine doNewTextscriptTerminator
  jsr doNewTextscriptTerminator
  jmp $E556
.ends

.section "new textscript terminator logic 2" APPENDTO "new textscript area"
  doNewTextscriptTerminator:
    ; make up work
    ; get previous script pos from call stack
    phx
      lda textscrCallStackPos_array.w,X
      tax
      
      dex
      lda textscrCallStack_array.w,X
      sta zp_textscriptPtr+1.b
      
      dex
      lda textscrCallStack_array.w,X
      sta zp_textscriptPtr+0.b
      
      txa
    plx
    sta textscrCallStackPos_array.w,X
    
    ; preemptively clear new area flag (we'll set it below if needed)
    lda newWindowFlags_array.w,X
    and #~savedNewWindowFlags
    sta newWindowFlags_array.w,X
    
    ; repeat above process for text area flag stack
    phx
      ; decrement stack target (to point to last entry)
      dec textscrNewTextAreaFlagStackPos_array.w,X
      ; get target position in stack
      lda textscrNewTextAreaFlagStackPos_array.w,X
      tax
      
      ; get flag from stack
      lda textscrNewTextAreaFlagStack_array.w,X
    plx
    ; OR with flags entry
    ora newWindowFlags_array.w,X
    sta newWindowFlags_array.w,X
    
    ; load appropriate banks for whatever we're returning into
    jmp loadTextBanksForCurrentTextscript
.ends

;=====
; delay
;=====

.bank metabank_bank00
.section "new textscript delay logic 1" orga $E5F9 overwrite
  jsr doNewTextscriptDelay
.ends

.section "new textscript delay logic 2" APPENDTO "new textscript area"
  doNewTextscriptDelay:
    ; if delay value is < 128, double it (to compensate for doubled
    ; text speed).
    ; delays beyond this length have been manually modified in the scripts.
    bmi +
      asl
    +:
    sta windowPrintDelay_array.w,X
    rts
.ends

;==================================
; general textscript routines
;==================================

.section "textscript bankswitch routines" APPENDTO "new textscript area"
  loadTextBanksForCurrentTextscript:
    lda newWindowFlags_array.w,X
    and #newWindowFlags_inNewTextArea
    bne @loadNewArea
    @loadOrigArea:
      bra loadOrigTextAreaBanks
    @loadNewArea:
      bra loadNewTextAreaBanks
    
  loadOrigTextAreaBanks:
    lda #$08
  loadAreaBanks_basedAtA:
    tam #$04
    ina
    tam #$08
    ina
    tam #$10
    ina
    tam #$20
    rts
    
  loadNewTextAreaBanks:
    lda #newTextBaseBank
    tam #$04
    ina
    tam #$08
    ina
    tam #$10
    ina
    tam #$20
    rts
.ends

;==================================
; new logic in default vblank
; handler
;==================================

makeSectionInFreeBank "new sprite definitions"
  newSpriteDefinitions:
.ends

.bank metabank_bank00
.section "new default vblank handler logic 1" orga $FD49 overwrite
  trampolineToCodeBankRoutine doNewDefaultVblankUpdate
  rts
.ends

.section "new default vblank handler logic 2" APPENDTO "new textscript area"
  doNewDefaultVblankUpdate:
    ; update all textscripts
    jsr doNewTextscriptUpdate
    
    ; draw sprites
    tma #$04
    pha
    tma #$08
    pha
    tma #$20
    pha
      lda #$02
      tam #$04
      ina
      tam #$08
      lda #(freeBankBaseBank+((:newSpriteDefinitions)-metabank_freebank_0))
      tam #$20
      
      stz currentSpriteCount.w
      clx
      -:
        jsr generateSatEntry
        inx
        cpx $2587.w
        bne -
      jsr transferSatEntries
    pla
    tam #$20
    pla
    tam #$08
    pla
    tam #$04
    
    rts
.ends

;==================================
; new textscript update
;==================================

.macro makeTwoPartLabel args first,second
  \1\2:
.endm

; .bank metabank_bank00
; .section "new textscript update 1" orga $E2F4 overwrite
;   trampolineToCodeBankRoutine doNewTextscriptUpdate
;   jmp $E32E
; .ends

.section "new textscript update 2" APPENDTO "new textscript area"
  doNewTextscriptUpdate:
    tma #$04
    pha
    tma #$08
    pha
    tma #$10
    pha
    tma #$20
    pha
  
      ; count number of active textscripts which may actually be printing
      ; content (bit 7 set)
      ; ...or opening or closing (states 01/02)
      cla
      clx
      -:
        tst #$83,textscrState_array.w,X
        beq +
          ina
        +:
        inx
        cpx #8
        bne -
      sta activeTextscriptCount.w
      
      clx
      -:
        ; load appropriate text banks for target
        jsr loadTextBanksForCurrentTextscript
      
        ; scale iteration count according to number of active textscripts
        ; (fewer active = more iterations for each)
        lda activeTextscriptCount.w
        ; 4 iterations if >= this
        cmp #3
        bcs @iteration4
  ;       ; 5 iterations if >= this
  ;       cmp #3
  ;       bcs @iteration3
        ; 6 iterations if >= this
        cmp #2
        bcs @iteration2
        ; otherwise, 8 iterations
        
        ; run 4 iterations of all textscripts
        .rept 8 INDEX count
          makeTwoPartLabel @iteration,count
          
          jsr runSingleTextscrIteration
          
          ; HACK: stop iterating if first half of a window restore completed.
          ; we don't want to execute the second half until the next frame.
          .if (count != 7)
            lda printing_windowRestoreSubState_array.w,X
            bne @iterationSetEnd
          .endif
        .endr
        @iterationSetEnd:
        
        inx
        cpx #$08
        bne -
      
      ; set flag indicating whether ANY textscript has a given flag set
      lda textscrState_array+0.w
      .rept 7 INDEX count
        ora textscrState_array+1+count.w
      .endr
      sta zp_allTextScriptsCombinedState.b
      
      ; if all windows are closed, reset print tile allocation state
      bne +
        ; check flag indicating this has already happened and no allocations
        ; have taken place since
        lda lastAllocatedPrintIndex+1.w
        cmp #$FF
        beq +
          ; set flag
          lda #$FF
          sta lastAllocatedPrintIndex+1.w
          jsr freeAllPrintTiles
      +:
    pla
    tam #$20
    pla
    tam #$10
    pla
    tam #$08
    pla
    tam #$04
    rts
.ends

;==================================
; scale print speed by iteration
; count
;==================================

.bank metabank_bank00
.section "addTextSpeedToIdleCounter scaling 1" orga $E901 overwrite
  jmp doNewAddTextSpeedToIdleCounter
.ends

.section "addTextSpeedToIdleCounter scaling 2" APPENDTO "new textscript area"
  doNewAddTextSpeedToIdleCounter:
    ; if print speed is 0x80 or greater, simply treat as 0xFF
    
    lda windowPrintSpeed_array.w,X
    bpl +
      lda #$FF
      bra @done
    +:
    
    ; DEBUG: if button ii pressed, force fastest text speed
    .if DEBUG_ENABLEFASTFORWARD != 0
      ; do not allow fast-forward if line interrupt handler is anything other
      ; than $E2E7 (the default, which does nothing).
      ; advancing text too quickly can cause late or missed interrupts with
      ; corresponding glitches.
      lda $4A.b
      cmp #<$E2E7
      bne @fastForwardCheckDone
      lda $4B.b
      cmp #>$E2E7
      bne @fastForwardCheckDone
      
      lda zp_buttonsPressed.b
      and #$02
      beq +
        lda #$FF
        bra @done
      +:
      @fastForwardCheckDone:
    .endif
    
;    phy
      lda activeTextscriptCount.w
      cmp #3
      bcs @normal
      cmp #2
      bcs @scale2
      @scale1:
        ; double textscript iterations = double speed.
        ; we double text speed by default anyway, so don't modify
        ; the speed value.
        lda windowPrintSpeed_array.w,X
        bra @scaleDone
      @scale2:
        ; 1.5x speed iteration
        ; FIXME: this isn't mathematically correct but should be fine.
        ; i think what we'd actually have to do here is multiply by 1.333,
        ; i.e. divide the base value by 3 and add it to the original,
        ; which probably isn't worth the computation time (especially
        ; since we have no decimal precision and the results will be
        ; inaccurate anyway).
        ; honestly, i may just ditch the 6-iteration mode if it turns out
        ; to cause any noticeable issues.
        
        lda windowPrintSpeed_array.w,X
        lsr
        clc
        adc windowPrintSpeed_array.w,X
        bra @scaleDone
      @normal:
        ; double speed
        lda windowPrintSpeed_array.w,X
        asl
      @scaleDone:
;    ply
    
    @done:
    clc
    adc windowPrintCharDelay_array.w,X
    sta windowPrintCharDelay_array.w,X
    rts
.ends

;==============================================================================
; bank 01
;==============================================================================

;==================================
; name -> std font format mapping
;==================================

.bank metabank_bank01
.section "convertRawToScriptText 1" orga $D4F5 SIZE $D2 overwrite
  ; $DC = src
  ; $DE = dst
  convertRawToScriptText:
;    clx
    cly
    
    ; if first character of input is 0xFF, output is always "censored"
    ; script data
    lda ($DC.b)
    cmp #$FF
    bne +
    @censor:
      lda censorNameScriptData,Y
      sta ($DE.b),Y
      beq @censorDone
      iny
      bra @censor
      @censorDone:
      rts
    +:
    
    ; convert input to script format
    @loop:
      ; fetch next char
      lda ($DC.b),Y
      beq @done
      
      ; remove any page bit (we don't use character pages)
      and #$7F
      ; look up equivalent character
      tax
      lda rawToScriptText_convTable.w,X
      sta ($DE.b),Y
      iny
      bra @loop
    @done:
    ; write terminator
;    sxy
;    cla
    sta ($DE.b),Y
;    sxy
    rts
  
  rawToScriptText_convTable:
    .include "gen/name2std.inc"
  censorNameScriptData:
    .incbin "out/script/bank01/censor-data.bin"
.ends

; .bank metabank_bank01
; .section "convertRawToScriptText 2" free
;   censorNameScriptData:
;     .incbin "out/script/bank01/censor-data.bin"
; .ends

;==============================================================================
; new printing system
;==============================================================================

;==================================
; index -> tileid mapping
;==================================

; hmm, can't discount the possibility that we might want more than 256 tiles,
; so better not to use this 8-bit solution 

; .section "print tile mapping array lo 1" APPENDTO "new textscript area"
;   printTileMappingLo_array:
; .ends
; 
; .section "print tile mapping array hi 1" APPENDTO "new textscript area"
;   printTileMappingHi_array:
; .ends
; 
; .define numPrintTileSlots 0
; 
; .macro addPrintTileMappingEntry ARGS tileNum
;   .redefine addPrintTileMappingEntry_frontTileId tileNum
;   .redefine addPrintTileMappingEntry_backTileId tileNum|$1000
;   
;   .section "addPrintTileMappingEntry 1 \@" APPENDTO "print tile mapping array lo 1"
;     .db <addPrintTileMappingEntry_frontTileId
;     .db <addPrintTileMappingEntry_backTileId
;   .ends
;   
;   .section "addPrintTileMappingEntry 2 \@" APPENDTO "print tile mapping array hi 1"
;     .db >addPrintTileMappingEntry_frontTileId
;     .db >addPrintTileMappingEntry_backTileId
;   .ends
;   
;   .redefine numPrintTileSlots numPrintTileSlots+2
; .endm
; 
; .rept $3F INDEX count
;   addPrintTileMappingEntry $40+count
; .endr
; 
; .rept $40 INDEX count
;   addPrintTileMappingEntry $A1+count
; .endr
; 
; .section "print tile mapping array lo 2" APPENDTO "new textscript area"
;   printTileMappingLo_array_end:
; .ends
; 
; .section "print tile mapping array hi 2" APPENDTO "new textscript area"
;   printTileMappingHi_array_end:
; .ends
; 
; .print numPrintTileSlots,"\n"
; .fail

; .section "print tile mapping array 1" APPENDTO "new textscript area"
;   printTileMapping_array:
; .ends
; 
; .define numPrintTileSlots 0
; 
; .macro addPrintTileMappingEntry ARGS tileNum
;   .redefine addPrintTileMappingEntry_frontTileId tileNum
;   .redefine addPrintTileMappingEntry_backTileId tileNum|$1000
;   
;   .section "addPrintTileMappingEntry 1 \@" APPENDTO "print tile mapping array 1"
;     .dw addPrintTileMappingEntry_frontTileId
;   .ends
;   
;   .section "addPrintTileMappingEntry 2 \@" APPENDTO "print tile mapping array 1"
;     .dw addPrintTileMappingEntry_backTileId
;   .ends
;   
;   .redefine numPrintTileSlots numPrintTileSlots+2
; .endm
; 
; .rept $3F INDEX count
;   addPrintTileMappingEntry $40+count
; .endr
; 
; .rept $40 INDEX count
;   addPrintTileMappingEntry $A1+count
; .endr

; .macro addPrintTileMappingEntry ARGS tileNum
;   .redefine addPrintTileMappingEntry_frontTileId tileNum
;   .redefine addPrintTileMappingEntry_backTileId tileNum|$1000
;   
;   .section "addPrintTileMappingEntry 1 \@" APPENDTO "print tile mapping array 1"
;     .dw addPrintTileMappingEntry_frontTileId
;   .ends
;   
;   .section "addPrintTileMappingEntry 2 \@" APPENDTO "print tile mapping array 1"
;     .dw addPrintTileMappingEntry_backTileId
;   .ends
;   
; .endm

; .macro addPrintTileMappingEntry ARGS tileNum
;   .section "addPrintTileMappingEntry 1 \@" APPENDTO "print tile mapping array 1"
;     .dw tileNum
;   .ends
; .endm

.macro addPrintTileMappingEntryIfDefined ARGS index
;   .ifdef printTileMapping_\1
;     addPrintTileMappingEntry (printTileMapping_\1)
;   .endif
  
  .ifdef printTileMapping_\1
    .dw (printTileMapping_\1)
  .endif
.endm

.section "print tile mapping array 1" APPENDTO "new textscript area"
  printTileMapping_array:
    ; add all print tiles that have been defined to the map
;     .rept maxPossiblePrintTileMappings INDEX count
;       addPrintTileMappingEntryIfDefined count
;     .endr
    .rept numPrintTileSlots INDEX count
      addPrintTileMappingEntryIfDefined count
    .endr
  printTileMapping_array_end:
.ends

;==================================
; tileid -> index mapping
;==================================

.macro addPrintTileReverseMappingEntryIfDefined ARGS index
  .ifdef printTileReverseMapping_\1
    .dw (printTileReverseMapping_\1)
  .else
    ; gaps in table are filled with "no entry" marker
    .dw $FFFF
  .endif
.endm

.section "print tile reverse mapping array 1" APPENDTO "new textscript area"
  printTileReverseMapping_array:
    .rept maxPossiblePrintTileReverseMappings INDEX count
      addPrintTileReverseMappingEntryIfDefined count
    .endr
  printTileReverseMapping_array_end:
.ends

;==================================
; tile allocation
;==================================

.section "print tile allocation 1" APPENDTO "new textscript area"
  ; returns mainMem_scratchWord0 == index of newly allocated tile.
  ; WARNING: if no tiles are available, all tiles are immediately
  ; deallocated and a previously-allocated tile is returned, which may
  ; result in existing (visible) content being overwritten.
  ; trashes scratchword1
  allocPrintTile:
    ; if last allocated print tile is 0xFFFF, clear it to zero.
    ; this is a special value used to indicate that the allocation array
    ; has been automatically cleared because all textscripts were inactive.
;     lda lastAllocatedPrintIndex+0.w
;     and lastAllocatedPrintIndex+1.w
;     cmp #$FF
;     bne +
;       stz lastAllocatedPrintIndex+0.w
;       stz lastAllocatedPrintIndex+1.w
;     +:
    ; or even easier, only check the high byte, because we're never going
    ; to have 0xFF00 slots
    lda lastAllocatedPrintIndex+1.w
    cmp #$FF
    bne +
      stz lastAllocatedPrintIndex+0.w
      stz lastAllocatedPrintIndex+1.w
    +:
    
    phx
      ; FIXME: this will erroneously fail if the most recently allocated tile
      ; has since been deallocated, and also is now the only remaining
      ; unallocated tile.
      ; but that seems like a pretty unlikely scenario.
      
      @restart:
      
      ; copy old value+1 to scratch
      lda lastAllocatedPrintIndex+0.w
      ina
      php
        sta mainMem_scratchWord0+0.w
        lda lastAllocatedPrintIndex+1.w
      plp
      bne +
        ina
      +:
      sta mainMem_scratchWord0+1.w
      
      jsr getBitflagArrayOffsetAndMask
      
      @loop:
        ; save bitmask
        pha
        
          ; check if we've reached the end of the indices and need to loop
          ; to the start
          lda mainMem_scratchWord0+0.w
          cmp #<numPrintTileSlots
          bne +
;            .if (numPrintTileSlots >= $100)
            lda mainMem_scratchWord0+1.w
            cmp #>numPrintTileSlots
            bne +
;            .endif
              ; reset to zero
              stz mainMem_scratchWord0+0.w
              stz mainMem_scratchWord0+1.w
              ; reset bitflag array pos
              clx
              ; replace saved bitmask value
              pla
              lda #$01
              pha
          +:
        
          ; check if we've looped to where we began without finding a result
          lda mainMem_scratchWord0+0.w
          cmp lastAllocatedPrintIndex+0.w
          bne +
            lda mainMem_scratchWord0+1.w
            cmp lastAllocatedPrintIndex+1.w
            bne +
              ; destroy saved bitmask on stack
              pla
              ; return failure value
;               lda #$FF
;               sta mainMem_scratchWord0+0.w
;               sta mainMem_scratchWord0+1.w
;              bra @done
              jsr freeAllPrintTiles
              jmp @restart
          +:
        ; restore bitmask
        pla
        
        ; check target bit in allocation array
        bit printTileAlloc_array.w,X
        ; if unset, we're done
        beq @success
        
        ; otherwise, advance to next tile
        inc mainMem_scratchWord0+0.w
        bne +
          inc mainMem_scratchWord0+1.w
        +:
        
        ; shift bitmask left
        asl
        ; if we shifted the bit out, advance to next byte
        bcc +
          rol
          inx
;          lda #$01
        +:
        
        bra @loop
    @success:
    ; set bitflag in tile allocation array
    ora printTileAlloc_array.w,X
    sta printTileAlloc_array.w,X
    ; return value becomes last allocated index
    lda mainMem_scratchWord0+0.w
    sta lastAllocatedPrintIndex+0.w
    lda mainMem_scratchWord0+1.w
    sta lastAllocatedPrintIndex+1.w
    
    @done:
    plx
    rts
  
  freeAllPrintTiles:
    phx
      clx
      cla
      -:
        sta printTileAlloc_array.w,X
        inx
        cpx #printTileMappingAllocArraySize
        bne -
    plx
    rts
  
  ; mainMem_scratchWord0 = RAW TILE ID (not internal index number) to deallocate.
  ; if not both allocable and allocated, nothing happens.
  ; trashes mainMem_scratchWord0.
  freePrintTileFromRawTilemapEntry:
    ;=====
    ; determine if raw tile id is in allocable range and reject if not
    ;=====
    
    ; check high byte
    lda mainMem_scratchWord0+1.w
    ; HACK: all allocable tiles are always in $7XX range, so test for that
    ; specifically
    and #$07
    cmp #$07
    bne @fullyDone
    
    ; HACK: specifically filter for the most common stuff.
    ; this routine needs to be fast or we will miss line interrupts
    ; when e.g. closing a window and deallocating all its contents.
    lda mainMem_scratchWord0+0.w
    cmp #$40
    bcc @fullyDone
    cmp #$E0
    bcs @fullyDone
    
    phy
      
      ;=====
      ; prep for reverse map lookup
      ;=====
      
      ; shift bit 4 of high byte to bit 0
      lda mainMem_scratchWord0+1.w
      .rept 4
        lsr
      .endr
      ; clear any remaining palette info
      and #$01
      sta mainMem_scratchWord0+1.w
      
      ; multiply by 2
      asl mainMem_scratchWord0+0.w
      rol mainMem_scratchWord0+1.w
      
      ;=====
      ; do reverse map lookup
      ;=====
      
      ; zp_scratchWord0 = offset+base
      lda mainMem_scratchWord0+0.w
      clc
      adc #<printTileReverseMapping_array
      sta zp_scratchWord0+0.b
      lda mainMem_scratchWord0+1.w
      adc #>printTileReverseMapping_array
      sta zp_scratchWord0+1.b
      
      ; reject if index is $FFFF
      cly
      lda (zp_scratchWord0.b),Y
      pha
        iny
        and (zp_scratchWord0.b),Y
        cmp #$FF
        bne +
          ; failure
          pla
          bra @done
        +:
        lda (zp_scratchWord0.b),Y
        sta mainMem_scratchWord0+1.w
      pla
      sta mainMem_scratchWord0+0.w
      
      ; do internal free
      jsr freePrintTileByIndex
    
    @done:
    ply
    @fullyDone:
    rts
  
  ; mainMem_scratchWord0 = internal index number of tile to deallocate
  freePrintTileByIndex:
    phx
      ; get bitmask
      jsr getBitflagArrayOffsetAndMask
      ; mask off target bit
      eor #$FF
      and printTileAlloc_array.w,X
      sta printTileAlloc_array.w,X
    plx
    rts
    
  
  ; scratchWord0 = target index
  ; 
  ; returns X = offset from array start, A = mask for current pos
  ; trashes scratchWord1_lo
  getBitflagArrayOffsetAndMask:
    ; compute bytepos in bitflag array
;    lda lastAllocatedPrintIndex+1.w
    lda mainMem_scratchWord0+1.w
    sta mainMem_scratchWord1+1.w
    lda mainMem_scratchWord0+0.w
    ; divide by 8 to get byte pos
    .rept 3
      lsr mainMem_scratchWord1+1.w
      ror
    .endr
    pha
      ; use low 3 bits to get target mask flag
      lda mainMem_scratchWord0+0.w
      and #$07
      tax
      lda byteNumToBitMaskTable.w,X
    plx
    
    rts
  
  byteNumToBitMaskTable:
    .rept 8 INDEX count
      .db (1<<count)
    .endr
.ends

;==================================
; font data
;==================================

; .section "font data 1" APPENDTO "new textscript area"
;   newFont:
;     .incbin "out/font/std/font.bin"
;   newFontWidthTable:
;     .incbin "out/font/std/width.bin"
; .ends

;==================================
; initialization
;==================================

.section "print initialization 1" APPENDTO "new textscript area"
  ; X = target window id
  resetPrintingForNewLine:
    ; if window is not at an exact x-position, increment coarse x,
    ; so that any subsequent write will go to the next tile instead of
    ; overwriting one with content
;     lda newWindowFlags_array.w,X
;     and #newWindowFlags_backedUpPrinting
;     bne +
;       lda printing_fineX_array.w,X
;       bne +
;         inc windowPrintX_array.w,X
;     +:
    
    ; clear relevant flags
    lda newWindowFlags_array.w,X
    and #~(newWindowFlags_targetingBackCompBuffer|newWindowFlags_backedUpPrinting)
    sta newWindowFlags_array.w,X
    
    stz printing_fineX_array.w,X
    stz printing_prevChar_array.w,X
    
    ; clear pattern num
    ; TODO: probably not needed?
;     lda #$FF
;     sta printTargetIndexFrontLo_array.w,X
;     sta printTargetIndexFrontHi_array.w,X
;     sta printTargetIndexBackLo_array.w,X
;     sta printTargetIndexBackHi_array.w,X
    
    rts
  
  ; X = target window id
  clearCurrentPrintCompBuf:
    phx
    phy
;       lda newWindowFlags_array.w,X
;       and #newWindowFlags_targetingBackCompBuffer
;       bne @back
;       @front:
;         lda #<printCompFrontBuffer_array
;         sta zp_scratchWord0+0.b
;         lda #>printCompFrontBuffer_array
;         sta zp_scratchWord0+1.b
;         bra @finish
;       @back:
;         lda #<printCompBackBuffer_array
;         sta zp_scratchWord0+0.b
;         lda #>printCompBackBuffer_array
;         sta zp_scratchWord0+1.b
;       @finish:
;       
;       phx
;       ldx #fontCompBufferSize
;         -:
;           sta (zp_scratchWord0),Y
;           dex
;           bne -
;       plx
      
      ldy #fontCompBufferSize
      
      lda newWindowFlags_array.w,X
      and #newWindowFlags_targetingBackCompBuffer
      php
        txa
        aslByPowerOf2 fontCompBufferSize
        tax
      plp
      bne @back
      @front:
        -:
          stz printCompFrontBuffer_array,X
          inx
          dey
          bne -
        bra @finish
      @back:
        -:
          stz printCompBackBuffer_array,X
          inx
          dey
          bne -
      @finish:
      
    ply
    plx
    rts
.ends

;==================================
; composition
;==================================

.section "print composition 1" APPENDTO "new textscript area"
  ; A = char id
  ; X = target window index
  printNewTextChar:
    pha
      ; if fineX is zero, prepare a new tile for output
      lda printing_fineX_array.w,X
      bne +
        jsr advancePrintTilePos
      +:
    pla
    
    ; save raw character for use in later lookups (e.g. width, kerning)
    sta mainMem_scratchWord1_lo.w
    
    ; save current MPR5
    tma #$20
    sta mainMem_scratchWord2_lo.w
    
    ; page in bank containing target font
    phx
      lda printing_currentFont_array.w,X
      ; save font id for future use
      sta mainMem_scratchWord2_hi.w
      tax
      lda fontBankTable.w,X
      tam #$20
      
      ; zp_scratchWord0 = font base ptr
      lda fontDataLoTable.w,X
      sta zp_scratchWord0+0.b
      lda fontDataHiTable.w,X
      sta zp_scratchWord0+1.b
    
      ; subtract base font index (0x20) from raw char id
      lda mainMem_scratchWord1_lo.w
      sec
      sbc #fontBaseIndex
      pha
        ; multiply by 8 and add to base font pointer
        stz mainMem_scratchWord1_hi.w
        .rept 3
          asl
          rol mainMem_scratchWord1_hi.w
        .endr
    ;      sta mainMem_scratchWord1+0.w
        clc
        adc zp_scratchWord0+0.b
        sta zp_scratchWord0+0.b
        lda mainMem_scratchWord1_hi.w
        adc zp_scratchWord0+1.b
        sta zp_scratchWord0+1.b
      pla
      ; save (codepoint - baseIndex) for later use
      sta mainMem_scratchWord1_hi.w
    plx
    
    ;=====
    ; step 1: apply kerning
    ;=====
    
    ; check previous character.
    ; if zero, skip this step.
    lda printing_prevChar_array.w,X
    beq @kerningDone
    
    ; look up kerning between previous character and current
    sta mainMem_scratchWord0_lo.w
    lda mainMem_scratchWord1_lo.w
    phx
      ; get font id
      ldx mainMem_scratchWord2_hi.w
      jsr getCharKerning
    plx
    
    ; if kerning is zero, skip this step
    cmp #$00
    beq @kerningDone
    
    ; subtract kerning from fineX
    clc
    adc printing_fineX_array.w,X
    sta printing_fineX_array.w,X
    ; if no underflow, skip the remainder of this step
    bpl @kerningDone
    
    ; move back to previous tile:
    ; - set backed-up flag
    ; - flip front/back comp buffer flag
    ; - decrement coarseX
    ; - add 8 to underflowed fineX, which should put it in range 0-7
      
      lda newWindowFlags_array.w,X
      ; backedUpPrinting should technically be ORed and not EORed, but
      ; we should never reach this code if backedUpPrinting is already set
      eor #(newWindowFlags_backedUpPrinting|newWindowFlags_targetingBackCompBuffer)
      sta newWindowFlags_array.w,X
      
      dec windowPrintX_array.w,X
      
      lda #$08
      clc
      adc printing_fineX_array.w,X
      sta printing_fineX_array.w,X
    
    @kerningDone:
    
    ;=====
    ; step 2: compose left side of character with current buffer
    ;=====
    
    ; TODO: we could optimize to skip this when fine x is zero, but that will
    ; require taking extra care when we've backed up (as the flag will normally
    ; have been cleared by this point, so we won't know without explicitly
    ; recording that information separately).
    ; we do not want to replace the buffer if content may have been written
    ; to it by a previous character.
    
    ; get pointer to current comp buffer in zp_scratchWord1
    jsr getCurrentPrintCompBuffer
    
    ; clear shift-out buffer
    tii zeroes_8bytes,printing_shiftOutBuffer,8
    
    ; for each line (byte) of target font data:
    ; - shift right by fineX (saving shift-out to a fixed buffer)
    ; - OR shifted result with current comp buffer
    lda printing_fineX_array.w,X
;     bne +
;       ; if no shift, set src to raw font from rom rather than comp buffer
;       lda zp_scratchWord0+0.b
;       sta zp_scratchWord1+0.b
;       lda zp_scratchWord0+1.b
;       sta zp_scratchWord1+1.b
;       bra @step2_shiftDone
;     +:
      phy
      phx
        bne +
        @noShift:
          ; OR with existing content.
          ; we don't overwrite it in case we have backed-up material in
          ; the buffer that we need to compose with.
          cly
          -:
            lda (zp_scratchWord0.b),Y
            ora (zp_scratchWord1.b),Y
            sta (zp_scratchWord1.b),Y
            
            iny
            cpy #fontCompBufferSize
            bne -
          bra @step2_shiftFinish
        +:
        
        ; if shift is 5 or more, rotate left instead of right so we shift
        ; through fewer bits
        cmp #5
        bcc @step2_right
        @step2_left:
;           tax
;           lda fineXReverseArray.w,X
;           
;           tax
;           cly
;           -:
;             ; x = number of shifts to perform
; ;            ldx mainMem_scratchWord1_hi.w
;             ; FIXME: jesus christ, this went horribly wrong.
;             ; probably far slower than just right-shifting all the way.
;             ; get rid of this and rewrite from scratch.
;             
;             ; TODO: could be optimized by uploading a self-modifying
;             ; routine into RAM:
;             ;   bra $00
;             ;   .rept 3
;             ;     asl
;             ;     rol printing_shiftOutBuffer.w,X
;             ;   .endr
;             ;   rts
;             ; then modify the branch distance to ((3 - numShifts) * 4) before
;             ; calling
;             phx
;               lda (zp_scratchWord0.b),Y
;               --:
;                 asl
;                 sxy
;                   rol printing_shiftOutBuffer.w,X
;                 sxy
;                 dex
;                 bne --
;               ; swap shift buf content with comp buf
;               pha
;                 sxy
;                   lda printing_shiftOutBuffer.w,X
;                 sxy
;                 ora (zp_scratchWord1.b),Y
;                 sta (zp_scratchWord1.b),Y
;               pla
;               sxy
;                 sta printing_shiftOutBuffer.w,X
;               sxy
;             plx
;             
;             iny
;             cpy #fontCompBufferSize
;             bne -
            ; modify branch distance of RAM routine to correspond to target
            ; number of bit shifts
            tax
            ; the first 5 entries in the table are omitted, so offset the base
            ; address by that amount
            lda printShifter_left_branchDistArray-5.w,X
            sta RAM_printShifter_left+((ROM_printShifter_left@branchInstr+1)-ROM_printShifter_left).w
            
            clx
            cly
            jsr RAM_printShifter_left.w
          
          bra @step2_shiftFinish
        @step2_right:
;           tax
;           cly
;           -:
;             ; TODO: could be optimized by uploading a self-modifying
;             ; routine into RAM:
;             ;   bra $00
;             ;   .rept 4
;             ;     lsr
;             ;     ror printing_shiftOutBuffer.w,X
;             ;   .endr
;             ;   rts
;             ; then modify the branch distance to ((4 - numShifts) * 4) before
;             ; calling
;             phx
;               lda (zp_scratchWord0.b),Y
;               --:
;                 lsr
;                 sxy
;                   ror printing_shiftOutBuffer.w,X
;                 sxy
;                 dex
;                 bne --
;               ora (zp_scratchWord1.b),Y
;               sta (zp_scratchWord1.b),Y
;             plx
;             
;             iny
;             cpy #fontCompBufferSize
;             bne -
          ; modify branch distance of RAM routine to correspond to target
          ; number of bit shifts
          tax
          lda printShifter_right_branchDistArray.w,X
          sta RAM_printShifter_right+((ROM_printShifter_right@branchInstr+1)-ROM_printShifter_right).w
          
          clx
          cly
          jsr RAM_printShifter_right.w
      @step2_shiftFinish:
      plx
    
      ; save target character's converted index
      lda mainMem_scratchWord1_hi.w
      phx
        ; Y = saved index
        tay
        
        ; send composited tile to screen
        jsr send1bppContentToCurrentPrintTile
        jsr sendPrintTileToScreen
        
        ; zp_scratchWord1 = width table base
        ldx mainMem_scratchWord2_hi.w
        
        lda fontWidthLoTable.w,X
        sta zp_scratchWord1+0.b
        lda fontWidthHiTable.w,X
        sta zp_scratchWord1+1.b
        
        ; look up character's width and add to fineX
        lda (zp_scratchWord1.b),Y
      plx
      
      clc
      adc printing_fineX_array.w,X
      sta printing_fineX_array.w,X
    ply
    
    ; check fineX
    lda printing_fineX_array.w,X
    ; if result < 8, we're done
    cmp #8
    bcs +
      jmp @done
    +:
    
    ; increment coarse x
    php
      inc windowPrintX_array.w,X
    plp
    
    ; if result == 8, set to 0 and we're done
    bne +
      stz printing_fineX_array.w,X
      jmp @done
    +:
    
    ; otherwise, there is carried-out content to print
    
    ;=====
    ; step 3: copy right side of character to new buffer
    ;=====
    
    @step3:
    
    ; start new tile
    jsr advancePrintTilePos
    
    ; get pointer to current comp buffer in zp_scratchWord1
    jsr getCurrentPrintCompBuffer
    
    ; copy each line of shift-out to current comp buffer unaltered
    phx
    phy
      clx
      cly
      -:
        lda printing_shiftOutBuffer.w,X
        sta (zp_scratchWord1.b),Y
        
        inx
        iny
        cpy #fontCompBufferSize
        bne -
    ply
    plx
    
    ; send composited tile to screen
    jsr send1bppContentToCurrentPrintTile
    jsr sendPrintTileToScreen
    
    ; subtract 8 from fineX
    lda printing_fineX_array.w,X
    sec
    sbc #8
    sta printing_fineX_array.w,X
    
    ; if result < 8, we're done
    cmp #8
;     bcs +
;       bra @done
;     +:
    bcc @done
    
    ; increment coarse x
    php
      inc windowPrintX_array.w,X
    plp
    
    ; if result == 8, set to 0 and we're done
    bne +
      stz printing_fineX_array.w,X
      bra @done
    +:
    
    ; clear shift-out buffer.
    ; this simulates the unstored part of the character beyond 8 pixels in width
    ; being blank.
    tii zeroes_8bytes,printing_shiftOutBuffer,8
    ; loop until finished
    bra @step3
    
    @done:
    ; restore MPR5
    lda mainMem_scratchWord2_lo.w
    tam #$20
    rts
  
;   printShifter_right_table:
;     .dw printShifter_right@by1
;     .dw printShifter_right@by2
;     .dw printShifter_right@by3
;     .dw printShifter_right@by4
;     .dw printShifter_right@by5
;   
;   printShifter_right:
;     @by5:
;     lsr
;     ror printing_shiftOutBuffer.w,X
;     @by4:
;     lsr
;     ror printing_shiftOutBuffer.w,X
;     @by3:
;     lsr
;     ror printing_shiftOutBuffer.w,X
;     @by2:
;     lsr
;     ror printing_shiftOutBuffer.w,X
;     @by1:
;     lsr
;     ror printing_shiftOutBuffer.w,X
;     rts

  ; provides the target branch distance for the first instruction of
  ; ROM_printShifter_right for each number of bitshifts.
  ; note that entry zero is a dummy to speed up lookups.
  printShifter_right_branchDistArray:
    .rept 5 INDEX count
      .db ((4 - count) * 4)
    .endr
  
;   printShifter_left_branchDistArray:
;     .rept 4 INDEX count
;       .db ((4 - count) * 4)
;     .endr
  
  ; 5 = 3-bit shift = 0
  ; 6 = 2-bit shift = 4
  ; 7 = 1-bit shift = 8
  ; all other entries are omitted.
  ; USE LOCATION 5 BYTES BEFORE ARRAY START FOR INDEXING!
  printShifter_left_branchDistArray:
;     .rept 5
;       .db $00
;     .endr
    .rept 3 INDEX count
      .db ((count) * 4)
    .endr
  
  ; X = target window index
  advancePrintTilePos:
    ; - flip front/back comp buffer flag
    ; - if backed-up flag unset:
    ;   - read back the current target tile from VRAM.
    ;     deallocate it (no matter what it is), and allocate
    ;     a new tile that will replace it.
    ;   - this tile's internal id is written to printTargetIndexFrontLo/Hi_array
    ; - else if backed-up flag set:
    ;   - clear backed-up flag
    ; 
    ; - this does NOT increment coarseX! caller must handle that if needed
    
    ; flip front/back comp buffer flag
    lda newWindowFlags_array.w,X
    eor #newWindowFlags_targetingBackCompBuffer
    sta newWindowFlags_array.w,X
    
    ; check backed-up flag
    and #newWindowFlags_backedUpPrinting
    beq @normal
    @backedUp:
      ; clear backed-up flag, and otherwise do nothing
      lda newWindowFlags_array.w,X
      and #~newWindowFlags_backedUpPrinting
      sta newWindowFlags_array.w,X
      rts
    @normal:
    
    ; read back the raw tilemap id of the tile from VRAM that this window is
    ; currently targeting (and presumably about to overwrite)
    jsr setMARRWithX
    lda vdp_dataLo.w
    sta mainMem_scratchWord0+0.w
    lda vdp_dataHi.w
    sta mainMem_scratchWord0+1.w
    
    ; deallocate it
    jsr freePrintTileFromRawTilemapEntry
    
    ; allocate a new tile
    jsr allocPrintTile
    
    ; save newly allocated tile id to front or back array as appropriate
    lda newWindowFlags_array.w,X
    and #newWindowFlags_targetingBackCompBuffer
    bne @back
    @front:
      lda mainMem_scratchWord0+0.w
      sta printTargetIndexFrontLo_array.w,X
      lda mainMem_scratchWord0+1.w
      sta printTargetIndexFrontHi_array.w,X
      bra @tileSaveDone
    @back:
      lda mainMem_scratchWord0+0.w
      sta printTargetIndexBackLo_array.w,X
      lda mainMem_scratchWord0+1.w
      sta printTargetIndexBackHi_array.w,X
    @tileSaveDone:
    
    ; clear the composition array for the new tile
    jsr getCurrentPrintCompBuffer
    phy
      cly
      cla
      -:
        sta (zp_scratchWord1.b),Y
        iny
        cpy #fontCompBufferSize
        bne -
    ply
    
    rts
  
  ; X = target window index
  ; returns mainMem_scratchWord0 = internal tile index of current print tile
  getCurrentPrintTileIndex:
    lda newWindowFlags_array.w,X
    and #newWindowFlags_targetingBackCompBuffer
    bne @back
    @front:
      lda printTargetIndexFrontLo_array.w,X
      sta mainMem_scratchWord0+0.w
      lda printTargetIndexFrontHi_array.w,X
      sta mainMem_scratchWord0+1.w
      rts
    @back:
      lda printTargetIndexBackLo_array.w,X
      sta mainMem_scratchWord0+0.w
      lda printTargetIndexBackHi_array.w,X
      sta mainMem_scratchWord0+1.w
      rts
  
  ; X = target window index
  ; returns zp_scratchWord1 = pointer to current composition buffer
  getCurrentPrintCompBuffer:
    lda newWindowFlags_array.w,X
    and #newWindowFlags_targetingBackCompBuffer
    bne @back
    @front:
      lda #<printCompFrontBuffer_array
      sta zp_scratchWord1+0.b
      lda #>printCompFrontBuffer_array
      sta zp_scratchWord1+1.b
      bra @done
    @back:
      lda #<printCompBackBuffer_array
      sta zp_scratchWord1+0.b
      lda #>printCompBackBuffer_array
      sta zp_scratchWord1+1.b
    @done:
    
    ; multiply window num by buffer size
    txa
    aslByPowerOf2 fontCompBufferSize
    
    ; add to base buffer pos to get target
    clc
    adc zp_scratchWord1+0.b
    sta zp_scratchWord1+0.b
    cla
    adc zp_scratchWord1+1.b
    sta zp_scratchWord1+1.b
    
    rts
  
  zeroes_8bytes:
    .rept 8
      .db $00
    .endr
  
  fineXReverseArray:
    .rept 8 INDEX count
      .db (8-count)
    .endr
  
  ; mainMem_scratchWord0 = internal tile index
  ; returns mainMem_scratchWord0 = corresponding output tilemap id
  ; (sans conversions for palette, etc.)
  ; trashes zp_scratchWord0
  getOutputTilemapIdForPrintTileIndex:
    phy
      ; internal index *= 2
      asl mainMem_scratchWord0+0.w
      rol mainMem_scratchWord0+1.w
      
      ; add base array pos
      lda mainMem_scratchWord0+0.w
      clc
      adc #<printTileMapping_array
      sta zp_scratchWord0+0.b
      lda mainMem_scratchWord0+1.w
      adc #>printTileMapping_array
      sta zp_scratchWord0+1.b
      
      cly
      lda (zp_scratchWord0.b),Y
      sta mainMem_scratchWord0+0.w
      iny
      lda (zp_scratchWord0.b),Y
      sta mainMem_scratchWord0+1.w
    ply
    rts
  
  ; X = target window index
  ; zp_scratchWord1 = pointer to src buffer content
  ; in addition to updating the target tile in vram, this
  ; returns mainMem_scratchWord0 = output tile id after conversions
  ; trashes mainMem_scratchWord1_hi
  send1bppContentToCurrentPrintTile:
    jsr getCurrentPrintTileIndex
    jsr getOutputTilemapIdForPrintTileIndex
    
    lda mainMem_scratchWord0+1.w
    php
      ; apply palette and "page" conversion to upper byte
      .rept 4
        asl
      .endr
      sta mainMem_scratchWord0+1.w
      lda windowPrintTilemapHiByte_array.w,X
      and #~$10
      ora mainMem_scratchWord0+1.w
      sta mainMem_scratchWord0+1.w
      
      ; shift tile num left by 4 to get target vram address for tile
      lda mainMem_scratchWord0+0.w
      stz mainMem_scratchWord1+1.w
;       .rept 4
;         asl
;         rol mainMem_scratchWord1+1.w
;       .endr
;     plp
;     
;     ; add offset for target bitplane
;     beq +
;       ; even == target high bitplane
;       clc
;       adc #$08
;     +:
      .rept 1
        asl
        rol mainMem_scratchWord1+1.w
      .endr
    plp
    
    ; add offset for target bitplane
    bne +
      ina
    +:
    
    .rept 3
      asl
      rol mainMem_scratchWord1+1.w
    .endr
    
    ; set MARR/MAWR to target tile
    
;     st0 #vdp_reg_marr
;     sta vdp_dataLo.w
;     pha
;       lda mainMem_scratchWord1+1.w
;       sta vdp_dataHi.w
;     pla
    
    st0 #vdp_reg_mawr
    sta vdp_dataLo.w
    lda mainMem_scratchWord1+1.w
    ; HACK: always target page 0x07
    ora #$70
    sta vdp_dataHi.w
    
    ; copy content
    st0 #vdp_reg_vwr
    phy
      cly
      -:
        lda (zp_scratchWord1.b),Y
        ; bg pixels must have low bits set
        eor #$FF
        sta vdp_dataLo.w
        ; fg pixels must have high bits set
        eor #$FF
        sta vdp_dataHi.w
        
        iny
        cpy #8
        bne -
    ply
    
    rts
    
  ; X = target window index
  ; mainMem_scratchWord0 = tile id
  sendPrintTileToScreen:
    jsr setMAWRWithX
    lda mainMem_scratchWord0+0.w
    sta vdp_dataLo.w
    lda mainMem_scratchWord0+1.w
    sta vdp_dataHi.w
    rts
.ends

;==============================================================================
; use new printing system where needed
;==============================================================================

;==================================
; 
;==================================

.bank metabank_bank00
.section "drawTextChar: use new printing 1" orga $E672 size 3 overwrite
  ; TODO: this may actually never be called outside of the textscript
  ; update loop, in which case the needed code bank will already be loaded
  ; and we can skip the trampoline here
;  trampolineToCodeBankRoutine doNewDrawTextChar
;  rts
  jmp doNewDrawTextChar
.ends

.section "drawTextChar: use new printing 2" APPENDTO "new textscript area"
  ; A = target character index
  doNewDrawTextChar:
    ; TODO: check for various special characters that need to be printed
    ; from the original font in the original way: old space character
    ; (used for deletions), certain HUD elements, etc.
    ; if we encounter these, we need to advance to the next tile-aligned
    ; position and handle the printing as in the original game.
    
    ; handle characters not assigned to normal font
    cmp #fontStdStartIndex
    bcc @handleAsOrigFontChar
    cmp #fontStdEndIndex
    bcc +
    @handleAsOrigFontChar:
      jmp drawOrigFontChar
    +:
    
    pha
      jsr printNewTextChar
    pla
    
    ; save as previous character
    sta printing_prevChar_array.w,X
    
    rts
  
  ; A = char index
  ; X = target window
  drawOrigFontChar:
    pha
      ; if window is not at an exact x-position, increment coarse x,
      ; so that the write will go to the next tile instead of overwriting
      ; the most recent one
      
      ; if we're backed up into the previous pattern, advance a tile to
      ; move out of it
      lda newWindowFlags_array.w,X
      and #newWindowFlags_backedUpPrinting
      beq +
        inc windowPrintX_array.w,X
      +
      
      ; if we're not at an even pixel boundary, advance a tile
      lda printing_fineX_array.w,X
      beq +
        inc windowPrintX_array.w,X
      +:
      
      ; reset as needed for aligned position
      jsr resetPrintingForNewLine
      
;      jsr setMAWRWithX
      
      ; if auto tile dealloc not inhibited, free the target tile
      lda printing_inhibitAutoTileDealloc.w
      bne +
        jsr setMAWRandMARRWithX
        
        ; read back the tile we're about to overwrite
        lda vdp_dataLo.w
        sta mainMem_scratchWord0+0.w
        lda vdp_dataHi.w
        sta mainMem_scratchWord0+1.w
        
        ; deallocate it
        jsr freePrintTileFromRawTilemapEntry
        
        bra @setupDone
      +:
        jsr setMAWRWithX
      @setupDone:
    pla
    
    ; write tile
    sta vdp_dataLo.w
    lda windowPrintTilemapHiByte_array.w,X
    sta vdp_dataHi.w
    
    ; advance coarse x
    inc windowPrintX_array.w,X
    
    rts
.ends

;==================================
; deallocate tiles where needed
;==================================

;=====
; drawSpaceChars
;=====

.bank metabank_bank00
.section "drawSpaceChars: dealloc tiles 1" orga $E65F size 7 overwrite
  trampolineToCodeBankRoutine doNewDrawSpaceChars
  rts
.ends

.section "drawSpaceChars: dealloc tiles 2" APPENDTO "new textscript area"
  doNewDrawSpaceChars:
;     jsr setMAWRWithX
;     jsr setMARRWithX
    lda printing_inhibitAutoTileDealloc.w
    bne @orig
    
    @new:
      jsr setMAWRandMARRWithX
      
      -:
        ; read back the tile we're about to overwrite
        lda vdp_dataLo.w
        sta mainMem_scratchWord0+0.w
        lda vdp_dataHi.w
        sta mainMem_scratchWord0+1.w
      
        ; deallocate it
        jsr freePrintTileFromRawTilemapEntry
        
        ; overwrite with space
        lda #$20
        sta vdp_dataLo.w
        lda windowPrintTilemapHiByte_array.w,X
        sta vdp_dataHi.w
        
        dec $59.b
        bne -
      rts
    
    @orig:
      jsr setMAWRWithX
      
      -:
        ; overwrite with space
        lda #$20
        sta vdp_dataLo.w
        lda windowPrintTilemapHiByte_array.w,X
        sta vdp_dataHi.w
        
        dec $59.b
        bne -
      rts
    
  setMAWRandMARRWithX:
    lda windowY_array.w,X
    clc 
    adc windowPrintY_array.w,X
    sta $57.b
    cla 
    lsr $57.b
    ror 
    lsr $57.b
    ror 
    sta $56.b
    
    lda windowX_array.w,X
    clc 
    adc windowPrintX_array.w,X
    clc 
    adc $56.b
    
    phx
      ; save low byte to X
      tax
      
      ; write target addr to MAWR
      st0 #vdp_reg_mawr
      sta vdp_dataLo.w
      lda $57.b
      adc #$00
      sta vdp_dataHi.w
      
      ; write target addr to MARR
      st0 #vdp_reg_marr
      pha
        txa
        sta vdp_dataLo.w
      pla
      sta vdp_dataHi.w
    plx
    
    ; select vwr/vrr
    st0 #vdp_reg_vwr
    rts
.ends

;=====
; resetWindowingState
;=====

.bank metabank_bank00
.section "resetWindowingState: clear tile allocation 1" orga $E6AE overwrite
;   nonInt_trampolineToCodeBankRoutine doNewResetWindowingState
  nonInt_trampolineToCodeBankRoutine doNewResetWindowingState
.ends

;makeSectionInCodeBank "resetWindowingState: clear tile allocation 2"
; put this in the new textscript area so it can call freeAllPrintTiles
.section "resetWindowingState: clear tile allocation 2" APPENDTO "new textscript area"
  doNewResetWindowingState:
;     ; ensure interrupts are disabled (in case this ever gets called in a
;     ; situation where a window is still being processed and might attempt to
;     ; do an allocation while we are )
;     php
;     pla
;     and #$04
;     bne +
;       sei
;     +:
;     pha
;       
;     ; re-enable interrupts if they weren't already off
;     pla
;     bne +
;       cli
;     +:
;     
;     ; make up work
;     lda #$00
;     sta $60.b
;     lda #$30
;     rts
  ; make up work
  stz textscrState_array+6.w
  stz textscrState_array+7.w
  
  ; all windows are now flagged as closed, and therefore won't be processed.
  ; since we know nothing will attempt an allocation at this point, it's
  ; safe to call this to reset the allocation array.
  jsr freeAllPrintTiles
  
  ; reset all windows as for new line
  phx
    clx
    -:
      jsr resetPrintingForNewLine
      inx
      cpx #8
      bne -
  plx
  
  rts
.ends

;=====
; closeWindow
;=====

.bank metabank_bank00
.section "closeWindow: multi-frame distribution 1" orga $E71C overwrite
  jsr doNewCloseWindow
.ends

.bank metabank_bank00
.section "closeWindow: multi-frame distribution 2" free
  doNewCloseWindow:
    -:
      ; make up work
      jsr waitForVsync
      ; check if close operation actually finished (state became zero)
      lda textscrState_array.w,X
      ; loop until done
;      cmp #2
;      beq -
      bne -
    rts
.ends

;=====
; restoreWindowArea
;=====

; do not automatically flag window as closed.
; this is now handled by restoreWindowArea itself to facilitate multi-frame
; window restoration.
.bank metabank_bank00
.section "restoreWindowArea: no auto stage change 1" orga $E3A9 overwrite
  nop
  nop
  nop
.ends

; .bank metabank_bank00
; .section "restoreWindowArea: dealloc tiles 1" orga $E89E size 9 overwrite
;   trampolineToCodeBankRoutine doNewRestoreWindowArea
;   rts
; ;  jmp $E8C5
; .ends

.bank metabank_bank00
.section "restoreWindowArea: dealloc tiles 1" orga $E883 SIZE $4D overwrite
;  trampolineToCodeBankRoutine doNewRestoreWindowArea
;  rts
;   jmp doNewRestoreWindowArea

  ; make up work
  lda windowContentRestoreStackPosLo_array.w,X
  sta $60.b
  lda windowContentRestoreStackPosHi_array.w,X
  sta $61.b
  lda windowH_array.w,X
  sta $58.b
  stz windowPrintX_array.w,X
  lda windowW_array.w,X
  asl 
  sta $59.b
  stz windowPrintY_array.w,X
  
  lda printing_inhibitAutoTileDealloc.w
  bne +
    jmp doNewRestoreWindowArea_withDealloc
  +:
  jmp doNewRestoreWindowArea_withNoDealloc
.ends

.section "restoreWindowArea: dealloc tiles 2" APPENDTO "new textscript area"
;  doNewRestoreWindowArea:
  
  doNewRestoreWindowArea_withDealloc:
;     ; make up work
;     lda windowContentRestoreStackPosLo_array.w,X
;     sta $60.b
;     lda windowContentRestoreStackPosHi_array.w,X
;     sta $61.b
;     lda windowH_array.w,X
;     sta $58.b
;     stz windowPrintX_array.w,X
;     lda windowW_array.w,X
;     asl 
;     sta $59.b
;     stz windowPrintY_array.w,X

    ; TODO: could probably add an additional check here to see if line interrupt
    ; handler is something other than the default one that does nothing,
    ; and only do the distributed version if there's a chance that we'll
    ; actually cause issues with scrolling, etc.
    
    ; FIXME?: mult8 really should not be called in an interrupt handler,
    ; as non-interrupt code appears to use it as well.
    ; on the other hand, the original game does so anyway, perhaps relying
    ; on the absence of lag under such circumstances.
    
    ; compute total window size
    lda windowH_array.w,X
;    asl
    sta zp_mult8_in0.b
    lda windowW_array.w,X
    sta zp_mult8_in1.b
    jsr mult8
    
    ; if it exceeds a maximum threshold, this operation must be distributed
    ; over two frames (or this is already in the process of occurring)
    lda zp_mult8_out1.b
    cmp #$02
    bcc @notDistributed
      
      ; check windowRestoreSubState array to determine whether we are in the
      ; first or second half of the operation
      lda printing_windowRestoreSubState_array.w,X
      bne @secondHalf
      @firstHalf:
        jsr resetPrintingForNewLine
        
        ; divide window H by 2 to get actual target height for this frame
        lda $58.b
        lsr
        sta $58.b
        
        ; flag as second half for next iteration
        inc printing_windowRestoreSubState_array.w,X
        ; do not flag window as closed here!
        ; we want to return to this routine on the next frame
        bra @setupDone
      @secondHalf:
        lda $58.b
        ; compute size of previous operation in bytes ((H/2)*W)
        lsr
        pha
          sta zp_mult8_in0.b
          
          ; add previous operation's H to (previously zeroed) printY so
          ; MAWR/MARR calculations go to the correct position
          clc
          adc windowPrintY_array.w,X
          sta windowPrintY_array.w,X
          
          lda windowW_array.w,X
          asl
          sta zp_mult8_in1.b
          jsr mult8
          
          ; add size of previous restore to base restore pos
          lda zp_mult8_out0.b
          clc
          adc $60.b
          sta $60.b
          lda zp_mult8_out1.b
          adc $61.b
          sta $61.b
        pla
        ; compute remaining height from previous operation:
        ; (windowH-(windowH>>1))
        eor #$FF
        ina
        clc
        adc $58.b
        sta $58.b
        
        ; reset substate
        stz printing_windowRestoreSubState_array.w,X
        ; flag window as closed
        stz textscrState_array.w,X
        bra @setupDone
    @notDistributed:
      jsr resetPrintingForNewLine
      ; flag window as closed
      stz textscrState_array.w,X
    @setupDone:
    
    -:
      cly
      jsr setMAWRandMARRWithX
      
      --:
        ; free tile we're about to overwrite
        lda vdp_dataLo.w
        sta mainMem_scratchWord0+0.w
        lda vdp_dataHi.w
        sta mainMem_scratchWord0+1.w
        jsr freePrintTileFromRawTilemapEntry
        
        ; overwrite with old content
        lda ($60.b),Y
        sta vdp_dataLo.w
        iny 
        lda ($60.b),Y
        sta vdp_dataHi.w
        iny 
        cpy $59.b
        bne --
      tya 
      clc 
      adc $60.b
      sta $60.b
      lda $61.b
      adc #$00
      sta $61.b
      inc windowPrintY_array.w,X
      dec $58.b
      bne -
    ; moved from caller to simplify new multi-frame window restore process
    lda windowContentRestoreStackPosLo_array.w,X
    sta $60.b
    lda windowContentRestoreStackPosHi_array.w,X
    sta $61.b
    rts
      
  doNewRestoreWindowArea_withNoDealloc:
;     ; make up work
;     lda windowContentRestoreStackPosLo_array.w,X
;     sta $60.b
;     lda windowContentRestoreStackPosHi_array.w,X
;     sta $61.b
;     lda windowH_array.w,X
;     sta $58.b
;     stz windowPrintX_array.w,X
;     lda windowW_array.w,X
;     asl 
;     sta $59.b
;     stz windowPrintY_array.w,X
    
    ; reset printing state
    jsr resetPrintingForNewLine
    
    -:
      cly
      jsr setMAWRWithX
      
      --:
        ; overwrite with old content
        lda ($60.b),Y
        sta vdp_dataLo.w
        iny 
        lda ($60.b),Y
        sta vdp_dataHi.w
        iny 
        cpy $59.b
        bne --
      tya 
      clc 
      adc $60.b
      sta $60.b
      lda $61.b
      adc #$00
      sta $61.b
      inc windowPrintY_array.w,X
      dec $58.b
      bne -
    ; moved from caller to simplify new multi-frame window restore process
    stz textscrState_array.w,X
    lda windowContentRestoreStackPosLo_array.w,X
    sta $60.b
    lda windowContentRestoreStackPosHi_array.w,X
    sta $61.b
    rts
.ends

;==================================
; don't increment printX when
; using drawTextChar, as
; drawTextChar now handles that
; when needed
;==================================

; ;=====
; ; main literal handler
; ;=====
; 
; ; NOTE: this also disables diacritic handling
; 
; .bank metabank_bank00
; .section "no drawTextChar auto x increment 1" orga $E415 overwrite
;   jmp $E42F
; .ends

;=====
; print2222BufferContents
;=====

.bank metabank_bank00
.section "no drawTextChar auto x increment 2" orga $E528 overwrite
  nop
  nop
  nop
.ends

;=====
; op 0x01 (printChar)
;=====

.bank metabank_bank00
.section "no drawTextChar auto x increment 3" orga $E56E overwrite
  nop
  nop
  nop
.ends

;=====
; op 0x09 (space padding)
;=====

.bank metabank_bank00
.section "no drawTextChar auto x increment 4" orga $E587 overwrite
  nop
  nop
  nop
.ends

;=====
; handle53AsOpOrLiteral
;=====

.bank metabank_bank00
.section "no drawTextChar auto x increment 5" orga $E396 overwrite
  nop
  nop
  nop
.ends

;==================================
; reset printing whenever print
; position is manually changed
;==================================

;=====
; setpos text op
;=====

.bank metabank_bank00
.section "setpos print reset 1" orga $E597 overwrite
  jsr setpos_doNewLogic
.ends

.section "setpos print reset 2" APPENDTO "new textscript area"
  setpos_doNewLogic:
    ; make up work
    sta windowPrintX_array.w,X
    
    jmp resetPrintingForNewLine
.ends

;=====
; resetPos
;=====

.bank metabank_bank00
.section "resetpos print reset 1" orga $E5E1 overwrite
  jmp resetpos_doNewLogic
.ends

.section "resetpos print reset 2" APPENDTO "new textscript area"
  resetpos_doNewLogic:
    ; make up work
    sta windowPrintX_array.w,X
    jmp resetPrintingForNewLine
.ends

;=====
; incrementPrintX
;=====

.bank metabank_bank00
.section "incrementPrintX print reset 1" orga $E630 overwrite
;   trampolineToCodeBankRoutine doNewIncrementPrintX
;   rts
  jmp doNewIncrementPrintX
.ends

.section "incrementPrintX print reset 2" APPENDTO "new textscript area"
  doNewIncrementPrintX:
    ; make up work.
    ; the original game checks for automatic line wrapping at the edge of
    ; the box here, but that's useless to us.
    inc windowPrintX_array.w,X
    
    jmp resetPrintingForNewLine
.ends

;=====
; decrementPrintX
;=====

.bank metabank_bank00
.section "decrementPrintX print reset 1" orga $E64A overwrite
  ; never used outside scripting system, so no need for trampoline
;  trampolineToCodeBankRoutine doNewDecrementPrintX
;  rts
  jmp doNewDecrementPrintX
.ends

.section "decrementPrintX print reset 2" APPENDTO "new textscript area"
  doNewDecrementPrintX:
    ; make up work.
    ; the original game checks for automatic line wrapping at the edge of
    ; the box here, but that's useless to us.
    dec windowPrintX_array.w,X
    
    jmp resetPrintingForNewLine
.ends

;=====
; drawWindowMoreTextIcon
;=====

.bank metabank_bank00
.section "drawWindowMoreTextIcon print reset 1" orga $E3AD SIZE $32 overwrite
  ; original implementation saves/restores printing position and prints
  ; target as a character.
  ; that's unworkable with the new printing system, so this is a rewrite.
  phy
  tay
    lda windowPrintY_array.w,X
    pha
    lda windowPrintX_array.w,X
    pha
    
      lda windowH_array.w,X
      dea
      sta windowPrintY_array.w,X
      
      lda windowW_array.w,X
      dea
      dea
      sta windowPrintX_array.w,X
      
      jsr setMAWRWithX
      
      jsr doNewDrawWindowMoreTextIcon
    pla
    sta windowPrintX_array.w,X
    pla
    sta windowPrintY_array.w,X
  ply
  
  rts
.ends

.section "drawWindowMoreTextIcon print reset 2" APPENDTO "new textscript area"
  doNewDrawWindowMoreTextIcon:
    ; A = target char to draw
    tya
    sta vdp_dataLo.w
    lda windowPrintTilemapHiByte_array.w,X
    ; force odd palette ("back page")
    ora #$10
    sta vdp_dataHi.w
    rts
.ends

;=====
; setPrintXyFrom4F
;=====

.bank metabank_bank00
.section "setPrintXyFrom4F print reset 1" orga $E755 SIZE 7 overwrite
  nonInt_trampolineToCodeBankRoutine doNewSetPrintXyFrom4F
  rts
.ends

.section "setPrintXyFrom4F print reset 2" APPENDTO "new textscript area"
  doNewSetPrintXyFrom4F:
    ; make up work
    lda $4F.b
    sta windowPrintY_array.w,X
    lda $50.b
    sta windowPrintX_array.w,X
    
    jmp resetPrintingForNewLine
.ends

;==============================================================================
; dynamic word wrap
;==============================================================================

;==================================
; use where needed
;==================================

.bank metabank_bank00
.section "use dynamic word wrap 1" orga $E410 overwrite
  jsr doCharPrintWithWordWrap
  ; skip x-increment and diacritic handling
  jmp $E42F
.ends

.section "use dynamic word wrap 2" APPENDTO "new textscript area"
  doCharPrintWithWordWrap:
    ; check next char
    cmp #textchar_space
    bne @notSpace
      ; HACK: to avoid having to do a full font lookup just to get the width
      ; of the space character, grab it from a premade table generated for this
      ; purpose
      pha
        ; temporarily set "previous character" to the not-yet-printed space
        ; so that kerning will be correctly computed
        lda printing_prevChar_array.w,X
        pha
          lda #textchar_space
          sta printing_prevChar_array.w,X
          
          phx
            lda printing_currentFont_array.w,X
            tax
            lda widthOfSpaceByFont.w,X
          plx
          
          jsr willNextWordFitOnLine
        pla
        sta printing_prevChar_array.w,X
      pla
      
      ; if next word will fit, print normally
      bcc @standard
      
      ; if not, do newline instead of printing character
      ; NOTE: we assume only one such character divides words
      jsr doNewTextscriptNewline
      ; carriage return
      jmp $E5B1
    @notSpace:
    
    cmp #textchar_exclaim
    beq @notVisibleWordDivider
    cmp #textchar_question
    beq @notVisibleWordDivider
      ; check prev char
      pha
        lda printing_prevChar_array.w,X
        cmp #textchar_ellipse
        beq @doPrevCharWrapCheck
        cmp #textchar_hyphen
        beq @doPrevCharWrapCheck
        cmp #textchar_emdash
        bne @noPrevCharWrap
        @doPrevCharWrapCheck:
          ; TODO: temporarily set prevChar to new char, then get width of current
          ; char in A.
          ; or back up input pos by 1, which should be fine.
          
          ; HACK: temporarily set previous char to null so that getNextWordWidth
          ; doesn't see that previous character is a visible divider and stop.
          ; then back up src by a char and run normal calculations.
          ; this assumes no kerning on any visible word divider.
          ; (even if there is, in the worst case, it just means that we'll break
          ; the line even if the remaining portion would actually fit within
          ; a margin of a few pixels.)
          
          ; FIXME: can Y ever be zero in this situation?
          cpy #0
          beq @noPrevCharWrap
          dey
          
          pha
          stz printing_prevChar_array.w,X
            cla
            jsr willNextWordFitOnLine
            bcc +
              ; newline
              jsr doNewTextscriptNewline
              ; carriage return
              jsr $E5B1
            +:
          pla
          sta printing_prevChar_array.w,X
          
          ; restore old Y
          iny
        @noPrevCharWrap:
      pla
    @notVisibleWordDivider:
    
    @standard:
    ; print normally
    jmp doNewDrawTextChar
  
  ; A = "initial width" of word.
  ;     this amount is added to the actual calculated width (to allow for the
  ;     case where we are considering an initial space, etc., toward the
  ;     upcoming word's width)
  ; returns carry clear if next word will fit on line, set otherwise
  willNextWordFitOnLine:
    ; compute width of next word
    jsr getNextWordWidth
    
    ; check if word will fit on current line
    lda windowPrintX_array.w,X
    asl
    asl
    asl
    clc
    adc printing_fineX_array.w,X
    
    clc
    adc mainMem_scratchWord1_lo.w
    ; if total width of line exceeds 256, assume it won't fit
    bcs @noFit
    
    clc
    adc #7
    bcs @noFit
    lsr
    lsr
    lsr
    
;    ina
    cmp windowW_array.w,X
    bcs @noFit
    
;     ina
;     cmp windowW_array.w,X
;     bcs @noFit
    
    @fit:
    clc
    rts
    @noFit:
    sec
    rts
.ends

;==================================
; word width calculations
;==================================

.section "getNextWordWidth 1" APPENDTO "new textscript area"
;   ; A = "initial width" of word
;   ; Y = offset into src at which to begin check
;   ; $5E-5F = src
;   ; returns mainMem_scratchWord1_lo = result
;   ; trashes mainMem_scratchWord1_hi
;   getNextWordWidth:
; ;    stz mainMem_scratchWord1_lo.w
;     sta mainMem_scratchWord1_lo.w
;     stz getNextWordWidth_step_recursionLevel
; ;    jmp getNextWordWidth_step
;     ; !!! drop through !!!
;   ; $5E-5F = src
;   ; Y = offset into src at which to begin check
;   ; returns mainMem_scratchWord1_lo = result
;   ; trashes mainMem_scratchWord1_hi
;   getNextWordWidth_step:
;     phy
;     lda zp_textscriptPtr+0.b
;     pha
;     lda zp_textscriptPtr+1.b
;     pha
;     lda newWindowFlags_array.w,X
;     pha
;     lda printing_prevChar_array.w,X
;     pha
;     lda printing_currentFont_array.w,X
;     pha
; ;     lda textscrCallStackPos_array.w,X
; ;     pha
;     
;     @loop:
;       jsr fetchWidthOfNextCharInWord
;       cmp #$FE
;       bne @notDefiniteWordEnd
;       @definiteWordEnd:
;         ; ???
; ;         bra @done
;         jmp @done
;       @notDefiniteWordEnd:
;       cmp #$FF
;       bne @notCallerHandledSeq
;       @callerHandledSeq:
;         ; check for and follow jumps, etc.
;         lda mainMem_scratchWord1_hi.w
;         
;         ; terminator
;         cmp #textop_end
;         bne +
;           jmp @done
; ;           ; if recursion level is nonzero, this is the end of a call we
; ;           ; followed ourself, and we're done (with this recursive step)
; ;           lda getNextWordWidth_step_recursionLevel.w
; ;           beq ++
; ;           @terminator_finished:
; ;             jmp @done
; ;           ++:
; ;           
; ;           ; if call stack position is the default, we've reached the end of
; ;           ; the script and are done
; ;           txa
; ;           asl
; ;           asl
; ;           cmp textscrCallStackPos_array.w,X
; ;           beq @terminator_finished
; ;           
; ;           ; otherwise, this is the end of a call which we did NOT trigger.
; ;           ; in other words, a called script contained a word divider which
; ;           ; triggered next-word-length evaluation, and we have now reached
; ;           ; the end of that script and need to go to the next level up
; ;           ; in order to evaluate the content there to find the end
; ;           ; of the word.
; ;           
; ;           ; HACK: treat this scenario as a jump to the top word on the
; ;           ; call stack
;           
;         +:
;         
;         ; font change
;         cmp #textop_setFont
;         bne +
;           lda (zp_textscriptPtr.b),Y
;           sta printing_currentFont_array.w,X
;           iny
;           bra @loop
;         +:
;         
;         ; jump
;         cmp #textop_jump
;         bne +
;           jsr loadNewScriptPtr
;           jsr loadOrigTextAreaBanks
;           bra @loop
;         +:
;         
;         ; jumpnew
;         cmp #textop_jumpNewText
;         bne +
;           jsr loadNewScriptPtr
;           jsr loadNewTextAreaBanks
;           bra @loop
;         +:
;         
;         ; calls
;         cmp #textop_call
;         beq @call
;         cmp #textop_call19
;         bne +
;         @call:
;           ; save old text pointer and bank setup
;           phy
;           lda zp_textscriptPtr+0.b
;           pha
;           lda zp_textscriptPtr+1.b
;           pha
;           tma #$04
;           pha
;             jsr loadNewScriptPtr
;             jsr loadOrigTextAreaBanks
;             ; do recursive call
;             ; (recursion is guaranteed to be 2 calls deep at most)
;             inc getNextWordWidth_step_recursionLevel.w
;             jsr getNextWordWidth_step
;             dec getNextWordWidth_step_recursionLevel.w
;           pla
;           jsr loadAreaBanks_basedAtA
;           pla
;           sta zp_textscriptPtr+1.b
;           pla
;           sta zp_textscriptPtr+0.b
;           ply
;           
;           ; advance past call op
;           iny
;           iny
;           bra @loop
;         +:
;         
;         ; number conversion ops
;         cmp #textop_num3
;         bcc +
;         cmp #(textop_num5ff+1)
;         bcs +
;           jsr getNumOpWidth
;           bra @notCallerHandledSeq
;         +:
;         
;         ; should never reach this
;         bra @done
;       @notCallerHandledSeq:
;       
;       ; add character's width to total
;       clc
;       adc mainMem_scratchWord1_lo.w
;       sta mainMem_scratchWord1_lo.w
;       
;       ; stop if visible word divider reached
;       lda mainMem_scratchWord1_hi.w
;       cmp #textchar_ellipse
;       beq @visDivCheck
;       cmp #textchar_hyphen
;       beq @visDivCheck
;       cmp #textchar_emdash
;       bne @visDivCheckDone
;       @visDivCheck:
;         ; get next char
;         lda (zp_textscriptPtr.b),Y
;         ; ...!, —!, etc.
;         cmp #textchar_exclaim
;         beq @visDivCheckDone
;         ; ...?, —?, etc.
;         cmp #textchar_question
;         beq @visDivCheckDone
;         ; otherwise, this is the end of the word
;         bra @done
;       @visDivCheckDone:
;       jmp @loop
;     
;     @done:
; ;     pla
; ;     sta textscrCallStackPos_array.w,X
;     pla
;     sta printing_currentFont_array.w,X
;     pla
;     sta printing_prevChar_array.w,X
;     pla
;     sta newWindowFlags_array.w,X
;     pla
;     sta zp_textscriptPtr+1.b
;     pla
;     sta zp_textscriptPtr+0.b
;     ply
;     rts
  
  ; A = "initial width" of word
  ; Y = offset into src at which to begin check
  ; $5E-5F = src
  ; returns mainMem_scratchWord1_lo = result
  ; trashes mainMem_scratchWord1_hi
  getNextWordWidth:
    ; save initial width
    sta mainMem_scratchWord1_lo.w
    
    ; we now have to save EVERYTHING related to the current text position
    ; and printing state that could be modified during width evaluation:
    ; - Y (current offset into script pointer)
    ; - script pointer
    ; - new window flags
    ; - previous character
    ; - current font
    ; - call stack position
    ; - call stack content
    ; - flag stack position
    ; - flag stack content
    
    tma #$04
    pha
    
    ;=====
    ; save general printing state
    ;=====
    
    phy
    lda zp_textscriptPtr+0.b
    pha
    lda zp_textscriptPtr+1.b
    pha
    lda newWindowFlags_array.w,X
    pha
    lda printing_prevChar_array.w,X
    pha
    lda printing_currentFont_array.w,X
    pha
    
    ;=====
    ; save call stack state
    ;=====
    
    lda textscrCallStackPos_array.w,X
    pha
    
    stx mainMem_scratchWord1_hi.w
    txa
    asl
    asl
    tax
;     lda textscrCallStack_array.w,X
;     pha
;     .rept 3
;       inx
;       lda textscrCallStack_array.w,X
;       pha
;     .endr
    .rept 4 INDEX count
      lda textscrCallStack_array+count.w,X
      pha
    .endr
    
    ;=====
    ; save flag stack state
    ;=====
    
    ; X = saved window id
    ldx mainMem_scratchWord1_hi.w
    lda textscrNewTextAreaFlagStackPos_array.w,X
    pha
    
    txa
    asl
    tax
    lda textscrNewTextAreaFlagStack_array+0.w,X
    pha
    lda textscrNewTextAreaFlagStack_array+1.w,X
    pha
    
      ;=====
      ; do word length calculations
      ;=====
    
      ; X = saved window id
      ldx mainMem_scratchWord1_hi.w
      jsr getNextWordWidth_step
      ; after call, save window id again
      stx mainMem_scratchWord1_hi.w
    
    ;=====
    ; restore flag stack state
    ;=====
    
    txa
    asl
    tax
    
    pla
    sta textscrNewTextAreaFlagStack_array+1.w,X
    pla
    sta textscrNewTextAreaFlagStack_array+0.w,X
    
    ; X = saved window id
    ldx mainMem_scratchWord1_hi.w
    pla
    sta textscrNewTextAreaFlagStackPos_array.w,X
    
    ;=====
    ; restore call stack state
    ;=====
    
    txa
    asl
    asl
    tax
    
    .rept 4 INDEX count
      pla
      sta textscrCallStack_array+(3-count).w,X
    .endr
;     .rept 3
;       dex
;       pla
;       sta textscrCallStack_array.w,X
;     .endr
    
    ; X = saved window id
    ldx mainMem_scratchWord1_hi.w
    
    pla
    sta textscrCallStackPos_array.w,X
    
    ;=====
    ; restore general printing state
    ;=====
    
    pla
    sta printing_currentFont_array.w,X
    pla
    sta printing_prevChar_array.w,X
    pla
    sta newWindowFlags_array.w,X
    pla
    sta zp_textscriptPtr+1.b
    pla
    sta zp_textscriptPtr+0.b
    
    ply
    
    pla
    jsr loadAreaBanks_basedAtA
    rts
    
  ; $5E-5F = src
  ; Y = offset into src at which to begin check
  ; returns mainMem_scratchWord1_lo = result
  ; trashes mainMem_scratchWord1_hi
  getNextWordWidth_step:
;     phy
;     lda zp_textscriptPtr+0.b
;     pha
;     lda zp_textscriptPtr+1.b
;     pha
;     lda newWindowFlags_array.w,X
;     pha
;     lda printing_prevChar_array.w,X
;     pha
;     lda printing_currentFont_array.w,X
;     pha
    
    @loop:
      jsr fetchWidthOfNextCharInWord
      cmp #$FE
      bne @notDefiniteWordEnd
      @definiteWordEnd:
        ; ???
;         bra @done
        jmp @done
      @notDefiniteWordEnd:
      cmp #$FF
      bne @notCallerHandledSeq
      @callerHandledSeq:
        ; check for and follow jumps, etc.
        lda mainMem_scratchWord1_hi.w
        
        ; terminator
        cmp #textop_end
        bne +
          ; check call stack pos
          txa
          asl
          asl
          cmp textscrCallStackPos_array.w,X
          ; if call stack pos is default for this window, this is the end of
          ; the script, and we're done
          bne @isCallEnd
          @isScriptEnd:
            jmp @done
          @isCallEnd:
          
          ; otherwise, pull the previous script's address off the call stack
          ; and continue evaluation from that position.
          ; mercifully, we can call existing code for this,
          ; which will also take care of things like setting Y to the correct
          ; value for further evaluation.
          ; note that we have applied hacks to this code to take care of
          ; pulling from the flag stack, etc.
          jsr $E540
          bra @loop
        +:
        
        ; font change
        cmp #textop_setFont
        bne +
          lda (zp_textscriptPtr.b),Y
          sta printing_currentFont_array.w,X
          iny
          bra @loop
        +:
        
        ; jump
        cmp #textop_jump
        bne +
          jsr loadNewScriptPtr
          jsr loadOrigTextAreaBanks
          bra @loop
        +:
        
        ; jumpnew
        cmp #textop_jumpNewText
        bne +
          jsr loadNewScriptPtr
          jsr loadNewTextAreaBanks
          bra @loop
        +:
        
        ; calls
        cmp #textop_call
        beq @call
        cmp #textop_call19
        bne +
        @call:
          ; call existing (modified) code for handling call
          jsr doNewTextscriptCall
          bra @loop
        +:
        
        ; number conversion ops
        cmp #textop_num3
        bcc +
        cmp #(textop_num5ff+1)
        bcs +
          jsr getNumOpWidth
          bra @notCallerHandledSeq
        +:
        
        ; should never reach this
        bra @done
      @notCallerHandledSeq:
      
      ; add character's width to total
      clc
      adc mainMem_scratchWord1_lo.w
      sta mainMem_scratchWord1_lo.w
      
      ; stop if visible word divider reached
      lda mainMem_scratchWord1_hi.w
      cmp #textchar_ellipse
      beq @visDivCheck
      cmp #textchar_hyphen
      beq @visDivCheck
      cmp #textchar_emdash
      bne @visDivCheckDone
      @visDivCheck:
        ; get next char
        lda (zp_textscriptPtr.b),Y
        ; ...!, —!, etc.
        cmp #textchar_exclaim
        beq @visDivCheckDone
        ; ...?, —?, etc.
        cmp #textchar_question
        beq @visDivCheckDone
        ; otherwise, this is the end of the word
        bra @done
      @visDivCheckDone:
      jmp @loop
    
    @done:
;     pla
;     sta printing_currentFont_array.w,X
;     pla
;     sta printing_prevChar_array.w,X
;     pla
;     sta newWindowFlags_array.w,X
;     pla
;     sta zp_textscriptPtr+1.b
;     pla
;     sta zp_textscriptPtr+0.b
;     ply
    rts
  
  ; NOTE: to simplify this operation, we make several assumptions:
  ; - font will never change within the converted number
  ; - converted numbers will never contain word separators
  ; - digits have no kerning
  ;
  ; Y = current offset from textscript base to op params
  ; zp_textscriptPtr = pointer to text base
  ; mainMem_scratchWord1_hi.w = opcode
  ; returns A = width of number
  ; advances Y past op args
  getNumOpWidth:
    ; retrieve width of zero in current font.
    ; this is assumed to be the width of all digits.
    phx
      lda printing_currentFont_array.w,X
      tax
      lda widthOfZeroByFont.w,X
      
      sta mainMem_scratchWord0_lo.w
      
      ; if op is zero- or space-padded, we need only multiply the width of the
      ; zero by the size of the op
      ; FIXME: actually, space-padded values use the original 8-pixel space,
      ; so this doesn't work correctly. however, such values shouldn't be used
      ; when there's a chance of word wrap occurring anyway.
      lda mainMem_scratchWord1_hi.w
      cmp #textop_num3
      beq @fixedWidth_3
      cmp #textop_num3zero
      beq @fixedWidth_3
      cmp #textop_num5
      beq @fixedWidth_5
      cmp #textop_num5zero
      bne @notFixedWidth
      @fixedWidth_5:
        lda mainMem_scratchWord0_lo.w
        asl
        bra @fixedWidthFinish
      @fixedWidth_3:
        lda mainMem_scratchWord0_lo.w
        @fixedWidthFinish:
        asl
        clc
        adc mainMem_scratchWord0_lo.w
        
        plx
        ; advance past op args
        iny
        iny
        rts
      @notFixedWidth:
      
      ; zp_scratchWord1 = pointer to target value from script
      lda (zp_textscriptPtr.b),Y
      sta zp_scratchWord1+0.b
      iny
      lda (zp_textscriptPtr.b),Y
      sta zp_scratchWord1+1.b
      iny
      
      ; mainMem_scratchWord2 = target value
      ; (may be an overread if an 8-bit print, but no big deal)
      phy
        lda (zp_scratchWord1.b)
        sta mainMem_scratchWord2+0.w
        ldy #1
        lda (zp_scratchWord1.b),Y
        sta mainMem_scratchWord2+1.w
      ply
      
      lda mainMem_scratchWord1_hi.w
      cmp #textop_num3ff
      beq @varWidth3
      @varWidth5:
        ; initial width in characters
        lda #5
        sta mainMem_scratchWord0_hi.w
        
        lda mainMem_scratchWord2+0.w
        sec
        sbc #<10000
        lda mainMem_scratchWord2+1.w
        sbc #>10000
        bcs @varWidthDone
        
        ; 4
        dec mainMem_scratchWord0_hi.w
        
        lda mainMem_scratchWord2+0.w
        sec
        sbc #<1000
        lda mainMem_scratchWord2+1.w
        sbc #>1000
        bcs @varWidthDone
        
        ; 3
        dec mainMem_scratchWord0_hi.w
        
        lda mainMem_scratchWord2+0.w
        sec
        sbc #<100
        lda mainMem_scratchWord2+1.w
        sbc #>100
        bcs @varWidthDone
        
        ; 2
        bra @varWidth3_finish
      @varWidth3:
        ; initial width in characters
        lda #3
        sta mainMem_scratchWord0_hi.w
        
        lda mainMem_scratchWord2+0.w
        cmp #100
        bcs @varWidthDone
        
        @varWidth3_finish:
        ; 2
        dec mainMem_scratchWord0_hi.w
        
        lda mainMem_scratchWord2+0.w
        cmp #10
        bcs @varWidthDone
        
        ; 1
        dec mainMem_scratchWord0_hi.w
      @varWidthDone:
      
      ; multiply digit width by digit count
      lda mainMem_scratchWord0_lo.w
      -:
        dec mainMem_scratchWord0_hi.w
        beq @done
        clc
        adc mainMem_scratchWord0_lo.w
        bra -
      @done:
    plx
    
    rts
  
  ; returns A as: 
  ; - $FF if unable to fetch next character due to one of the following:
  ;   - null terminator
  ;     (may be end of word or end of subtext call; caller must handle)
  ;   - subtext call
  ;   - jump sequence
  ;   - number conversion op
  ; - $FE if end of word unambiguously reached
  ;   - reached a character that is considered word-terminating, such as a space
  ;   - reached an op that is considered word-terminating,
  ;     such as a space generator
  ; - width of next character otherwise.
  ;   irrelevant ops will be silently eaten.
  ; sets mainMem_scratchWord1_hi to the final checked byte.
  fetchWidthOfNextCharInWord:
    ; get next char
    lda (zp_textscriptPtr.b),Y
    sta mainMem_scratchWord1_hi.w
    iny
    
    ; save current MPR5
    tma #$20
    sta mainMem_scratchWord2_lo.w
    
    ; page in bank containing target font
    phx
      lda printing_currentFont_array.w,X
      ; save font id for future use
      sta mainMem_scratchWord2_hi.w
      tax
      lda fontBankTable.w,X
      tam #$20
    plx
    
    ; check read codepoint
    lda mainMem_scratchWord1_hi.w
    cmp #fontBaseIndex
    bcs @notOp
      phx
        tax
        lda wordWidthOpParamSizeTable.w,X
        
        ; if listed param size >= 0x80, return as-is
        bpl +
          plx
          jmp restoreSavedScriptBank
;          bra @wordEnd
        +:
        
        ; add size of params to Y
        tya
        clc
        adc wordWidthOpParamSizeTable.w,X
        tay
      plx
      ; loop to next char
      jsr restoreSavedScriptBank
      bra fetchWidthOfNextCharInWord
    @notOp:
    
    ; check for word-separating characters
    cmp #textchar_space
    bne @notStdWordEnd
    @stdWordEnd:
      lda #$FE
      jmp restoreSavedScriptBank
    @notStdWordEnd:
    
    ; check for visible word dividers in PREVIOUS character
    lda printing_prevChar_array.w,X
    cmp #textchar_ellipse
    beq @previousWasVisibleWordDivider
    cmp #textchar_hyphen
    beq @previousWasVisibleWordDivider
    cmp #textchar_emdash
    bne @notVisibleWordDivider
    @previousWasVisibleWordDivider:
      ; verify that CURRENT character is not something that would cause the
      ; previous character to be treated as non-dividable
;      dey
;      lda (zp_textscriptPtr.b),Y
;      iny
      lda mainMem_scratchWord1_hi.w
      
      ; ...!, —!, etc.
      ; (hyphen is not grammatically correct here, but whatever)
      cmp #textchar_exclaim
      beq @notVisibleWordDivider
      ; ...?, —?, etc.
      cmp #textchar_question
      beq @notVisibleWordDivider
      
        ; this is the end of the word
        lda #$FE
        jmp restoreSavedScriptBank
    @notVisibleWordDivider:
    
    @literal:
    lda mainMem_scratchWord1_hi.w
    
;     ; TEMP: discard unassigned characters
;     cmp #$80
;     bcc +
;       lda #$00
;       rts
;     +:
    
    ; any unassigned indices are treated as end of word
    ; (should not be used in autowrapping contexts)
    cmp #fontStdStartIndex
    bcc @handleAsOrigFontChar
    cmp #fontStdEndIndex
    bcc +
    @handleAsOrigFontChar:
      ; previous char is treated as null
      stz printing_prevChar_array.w,X
      lda #$FE
      jmp restoreSavedScriptBank
    +:
    
    ; look up kerning with previous character
    lda printing_prevChar_array.w,X
    beq @kerningDone
      sta mainMem_scratchWord0_lo.w
      lda mainMem_scratchWord1_hi.w
      phx
        ; get font id
        ldx mainMem_scratchWord2_hi.w
        jsr getCharKerning
      plx
    @kerningDone:
    
    ; add width from table
;     phx
;       ldx mainMem_scratchWord1_hi.w
;       clc
;       adc newFontWidthTable-fontBaseIndex.w,X
;     plx
    phy
      pha
      phx
        ; Y = converted char index
        lda mainMem_scratchWord1_hi.w
        sec
        sbc #fontBaseIndex
        tay
        
        ; get target font
        lda printing_currentFont_array.w,X
        tax
        ; zp_scratchWord1 = width table base for font
        lda fontWidthLoTable.w,X
        sta zp_scratchWord1+0.b
        lda fontWidthHiTable.w,X
        sta zp_scratchWord1+1.b
        
        ; look up character's width
;        lda (zp_scratchWord1.b),Y
      plx
      pla
      
      ; add to kerning
      clc
      adc (zp_scratchWord1.b),Y
    ply
    
    ; save current character as previous
    pha
      lda mainMem_scratchWord1_hi.w
      sta printing_prevChar_array.w,X
    pla
    ; !!!!! drop through !!!!!
  restoreSavedScriptBank:
    pha
      lda mainMem_scratchWord2_lo.w
      tam #$20
    pla
    rts
  
  ; FE = end of word
  ; FF = terminator, call, or jump
  ; otherwise, number of bytes needed to skip op's params
  wordWidthOpParamSizeTable:
    ; 00: end
    .db $FF
    ; 01: printchar
    .db $01
    ; 02: wait
    .db $00
    ; 03: blankCurrentPos
    .db $FE
    ; 04: setpos
    .db $FE
    ; 05: call
    .db $FF
    ; 06: speed
    .db $01
    ; 07: ?
    .db $01
    ; 08: originally dummy, now font change
;    .db $00
    .db $FF
    ; 09: tonext8
    .db $FE
    ; 0A: newline
    .db $FE
    ; 0B: decpos
    .db $FE
    ; 0C: incpos
    .db $FE
    ; 0D: carriage return
    .db $FE
    ; 0E: set page to back
    .db $00
    ; 0F: set page to front
    .db $00
    ; 10: num3
    .db $FF
    ; 11: num3zero
    .db $FF
    ; 12: num3ff
    .db $FF
    ; 13: num5
    .db $FF
    ; 14: num5zero
    .db $FF
    ; 15: num5ff
    .db $FF
    ; 16: dummy
    .db $00
    ; 17: jumpNewText
    .db $FF
    ; 18: jump
    .db $FF
    ; 19: call19
    .db $FF
    ; 1A: clear
    .db $FE
    ; 1B: various subops.
    ;     technically should be specially handled, but can probably just
    ;     treat as word terminator in practice.
    .db $FE
    ; 1C: eraseToLineEnd
    .db $FE
    ; 1D: delay
    .db $01
    ; 1E: resetPos
    .db $FE
    ; 1F: dummy
    .db $00
  
  ; mainMem_scratchWord0_lo = previous character
  ; A = current character
  ; X = target font index
  ; ** BANK(S) CONTAINING FONT SHOULD ALREADY BE LOADED **
  ; returns A = kerning between previous and current characters
  ; trashes mainMem_scratchWord0 and zp_scratchWord1
  getCharKerning:
    phx
    phy
      ; save current char
      sta mainMem_scratchWord0_hi.w
      
      ; zp_scratchWord1 = pointer to kerning index
      lda fontKerningIndexLoTable.w,X
      sta zp_scratchWord1+0.b
      lda fontKerningIndexHiTable.w,X
      sta zp_scratchWord1+1.b
      
      ; do nothing if prev char is unset (null)
      lda mainMem_scratchWord0_lo.w
      beq @done_null
      ; convert to indexed form
      ; (codepoints are actually stored in the data raw, so we don't want this)
;      sec
;      sbc #fontBaseIndex
;      sta mainMem_scratchWord0_lo.w
      
      ; do nothing if current char is unset (null)
      lda mainMem_scratchWord0_hi.w
      beq @done_null
      
      ; convert to indexed form
      sec
      sbc #fontBaseIndex
      
      ; look up offset to kerning data from index and add to base position
;       asl
;       tay
;       iny
;       lda (zp_scratchWord1.b),Y
;       pha
;         dey
;         lda (zp_scratchWord1.b),Y
;         clc
;         adc zp_scratchWord1+0.b
;         sta zp_scratchWord1+0.b
;       pla
;       adc zp_scratchWord1+1.b
;       sta zp_scratchWord1+1.b
      ; WARNING: we are implicitly limiting the maximum number of characters
      ; to 0x80 by doing this
      asl
      tay
      
      ; check if offset is 0xFFFF (no kerning with this character)
      lda (zp_scratchWord1.b),Y
      iny
      and (zp_scratchWord1.b),Y
      cmp #$FF
      beq @done_null
      
      ; add offset to base
      lda (zp_scratchWord1.b),Y
      pha
        dey
        lda (zp_scratchWord1.b),Y
        clc
        adc zp_scratchWord1+0.b
        sta zp_scratchWord1+0.b
      pla
      adc zp_scratchWord1+1.b
      sta zp_scratchWord1+1.b
      
      ; zp_scratchWord1 now points to the kerning data for this character.
      ; parse it to see if there's a match for the previous character.
      
      ; X = default kerning = -1
      ldx #$FF
      cly
      @checkLoop:
        ; fetch next byte
        lda (zp_scratchWord1.b),Y
        iny
        
        ; check for 0xF0 == terminator
        ; if terminator, assume null kerning
        cmp #$F0
        beq @done_null
        bcc @notIncrement
          ; kerning += signed value.
          ; this should rarely be anything other than 0xFF, so it's probably
          ; not worth doing this calculation via proper arithmetic.
          -:
            dex
            ina
            bne -
          bra @checkLoop
        @notIncrement:
        
        ; 0xE0-0xEF == repeat
        cmp #$E0
        bcc @notRepeat
          ; save upper value of range
          and #$0F
          clc
          adc #kerning_runLenBase
          sta mainMem_scratchWord0_hi.w
          
          ; get prev char
          lda mainMem_scratchWord0_lo.w
          ; subtract base value of run
          sec
          sbc (zp_scratchWord1.b),Y
          iny
          ; no match if prev char < range base
          bcc @checkLoop
          
          ; no match if >= upper value of range
          cmp mainMem_scratchWord0_hi.w
          bcs @checkLoop
          
          ; matched
          bra @matched
        @notRepeat:
        
        ; literal
        ; if ID of previous character, use current kerning
        cmp mainMem_scratchWord0_lo.w
        bne @checkLoop
    
      @matched:
      txa
      bra @done
    
    @done_null:
    cla
    
    @done:
    ply
    plx
    rts
.ends

;==================================
; 
;==================================

; HACK: this is stupid and essentially wastes a bunch of space in the
; "new textscript area", but if it works, i'm not inclined to spend more
; time on it.
; the censor script is explicitly referenced at $DB59 by textscripts in many
; places, but that refers to a location in bank01. during textscript processing,
; we have a different bank paged into MPR6 instead. by putting the same data
; at the same position, we avoid this issue.

.bank metabank_codebank_0
.section "censor replacement" orga $D5B9 FORCE
  .incbin "out/script/bank01/censor-data.bin"
.ends

;==============================================================================
; fonts
;==============================================================================

;==================================
; lookup tables for font resources
;==================================

.section "font bank table" APPENDTO "new textscript area"
  fontBankTable:
    
.ends

.section "font data lo table" APPENDTO "new textscript area"
  fontDataLoTable:
    
.ends

.section "font data hi table" APPENDTO "new textscript area"
  fontDataHiTable:
    
.ends

.section "font width lo table" APPENDTO "new textscript area"
  fontWidthLoTable:
    
.ends

.section "font width hi table" APPENDTO "new textscript area"
  fontWidthHiTable:
    
.ends

.section "font kerning index lo table" APPENDTO "new textscript area"
  fontKerningIndexLoTable:
    
.ends

.section "font kerning index hi table" APPENDTO "new textscript area"
  fontKerningIndexHiTable:
    
.ends

.section "font space width table" APPENDTO "new textscript area"
  widthOfSpaceByFont:
    
.ends

.section "font zero width table" APPENDTO "new textscript area"
  widthOfZeroByFont:
    
.ends

;==================================
; add font data
;==================================

.macro addFont ARGS name
  ; put data in a free bank
  makeSectionInFreeBank "font \1 rsrc"
    font_\1_data:
      .incbin "out/font/\1/font.bin"
    font_\1_width:
      .incbin "out/font/\1/width.bin"
    font_\1_kerningIndex:
      .incbin "out/font/\1/kerning_index.bin"
    font_\1_kerningData:
      .incbin "out/font/\1/kerning_data.bin"
  .ends
  
  ; add containing bank to font bank table
  .section "font \1 bank table entry" APPENDTO "font bank table" AUTOPRIORITY
    .db freeBankBaseBank+((:font_\1_data)-metabank_freebank_0)
  .ends
  
  ; add to data table
  
  .section "font \1 data lo table entry" APPENDTO "font data lo table" AUTOPRIORITY
    .db <(font_\1_data)
  .ends
  
  .section "font \1 data hi table entry" APPENDTO "font data hi table" AUTOPRIORITY
    .db >(font_\1_data)
  .ends
  
  ; add to width table
  
  .section "font \1 width lo table entry" APPENDTO "font width lo table" AUTOPRIORITY
    .db <(font_\1_width)
  .ends
  
  .section "font \1 width hi table entry" APPENDTO "font width hi table" AUTOPRIORITY
    .db >(font_\1_width)
  .ends
  
  ; add to kerning index table
  
  .section "font \1 kerning index lo table entry" APPENDTO "font kerning index lo table" AUTOPRIORITY
    .db <(font_\1_kerningIndex)
  .ends
  
  .section "font \1 kerning index hi table entry" APPENDTO "font kerning index hi table" AUTOPRIORITY
    .db >(font_\1_kerningIndex)
  .ends

  .section "font \1 space width table entry" APPENDTO "font space width table" AUTOPRIORITY
    .incbin "out/font/\1/width_of_space.bin"
  .ends

  .section "font \1 zero width table entry" APPENDTO "font zero width table" AUTOPRIORITY
    .incbin "out/font/\1/width_of_zero.bin"
  .ends
.endm

.macro addRegisteredFontByNum ARGS num
  addFont registeredFontName_\1
.endm

; add data for all registered fonts
.rept numRegisteredFonts INDEX count
  addRegisteredFontByNum count
.endr

;==============================================================================
; gamescripts
;==============================================================================

;==================================
; new protagonist default name
;==================================

.bank metabank_gamescript
.section "gamescript: protagonist default name 1" orga $B182 overwrite
  .dw protagonistDefaultName
.ends

.bank metabank_gamescript
.section "gamescript: protagonist default name 2" free
  protagonistDefaultName:
    .incbin "out/script/names/default-name.bin"
.ends

;==================================
; new shop cheat name
;==================================

; normal overwrite when cheat used
.bank metabank_gamescript
.section "gamescript: shop cheat name 1" orga $50B0 overwrite
  .dw shopCheatName
.ends

; FIXME: this is clearly doing the same thing as the other reference above.
; what's it used for?
.bank metabank_gamescript
.section "gamescript: shop cheat name 2" orga $6B7F overwrite
  .dw shopCheatName
.ends

.bank metabank_gamescript
.section "gamescript: shop cheat name 3" free
  shopCheatName:
    .incbin "out/script/names/shop-cheat-name.bin"
.ends

;==================================
; new license fail name
;==================================

; normal overwrite when cheat used
.bank metabank_gamescript
.section "gamescript: license fail name 1" orga $9269 overwrite
  .dw licenseFailName
.ends

.bank metabank_gamescript
.section "gamescript: license fail name 2" free
  licenseFailName:
    .incbin "out/script/names/license-fail-name.bin"
.ends

;==================================
; disable automatic tile
; deallocation during opening
; cutscene
;==================================

; the automatic tile deallocation interferes with the line interrupts used
; during the opening cutscene. the scene is totally linear, so we can
; just shut off deallocation for the duration, which effectively just results
; in circular allocation through the whole buffer.

;=====
; turn on at start
;=====

.bank metabank_gamescript
.section "gamescript: intro tile dealloc disable 1" orga $4139 overwrite
  gamescript_jump gamescr_introAllocHack1
.ends

.bank metabank_gamescript
.section "gamescript: intro tile dealloc disable 2" free
  gamescr_introAllocHack1:
    ; disable auto tile deallocation
    gamescript_writeByte printing_inhibitAutoTileDealloc,$01
    
    ; make up work
    gamescript_runText 0,$4000
    gamescript_jump $413D
.ends

;=====
; turn off at end
;=====

.bank metabank_gamescript
.section "gamescript: intro tile dealloc enable 1" orga $436A overwrite
  gamescript_jump gamescr_introAllocHack2
.ends

.bank metabank_gamescript
.section "gamescript: intro tile dealloc enable 2" free
  gamescr_introAllocHack2:
    ; enable auto tile deallocation
    gamescript_writeByte printing_inhibitAutoTileDealloc,$00
    
    ; make up work
    gamescript_runText 0,$41BD
    gamescript_jump $436E
.ends

;==============================================================================
; load uncompressed graphics/tilemaps where specified
;==============================================================================

; NOTE: unlike the majority of the modifications made in the project, this code
; (and anything else related to gamescripts) does not run out of the vsync
; interrupt handler. it therefore needs to use the alternate "nonInt" trampoline
; routines and scratch memory area, or an interrupt that occurs at a bad time
; may clobber ongoing work.

;==================================
; loadGraphicList
;==================================

.bank metabank_bank00
.section "loadGraphicList: uncompressed data check 1" orga $ED03 overwrite
  jmp loadGraphicList_uncmpCheck
.ends

.bank metabank_bank00
.section "loadGraphicList: uncompressed data check 2" free
  loadGraphicList_uncmpCheck:
    ; make up work
    lda ($70.b)
    bne +
      ; done
      jmp $ED17
    +:
    
    ; check if bit 0x80 of bank set (uncompressed)
    bpl +
      ; copy param list
      jsr copyGraphicLoadListParams
      
      ; load uncompressed graphic
      nonInt_trampolineToCodeBankRoutine loadUncompressedGrp
      
      ; resume loop
      jmp $ED0A
    +:
    
    ; handle as normal
    jmp $ED07
.ends

.bank metabank_bank00
.section "loadGraphicList: uncompressed data check 3" free
;   copyGraphicLoadListParams:
;     cly
;     -:
;       lda ($70.b),Y
;       sta mainMem_nonInt_scratchWord0.w,Y
;       iny
;       cpy #5
;       bne -
;     rts
  copyGraphicLoadListParams:
    ldy #1
    
    lda ($70.b),Y
    sta zp_nonInt_scratchWord1+0.b
    iny
    
    lda ($70.b),Y
    sta zp_nonInt_scratchWord1+1.b
    iny
    
    lda ($70.b),Y
    sta mainMem_nonInt_scratchWord2+0.w
    iny
    
    lda ($70.b),Y
    sta mainMem_nonInt_scratchWord2+1.w
    
    lda ($70.b)
;    sta zp_nonInt_scratchWord1+0.b
    
    rts
.ends

makeSectionInCodeBank "loadUncompressedGrp 1"
  ; A = bank num with bit 0x80 set
  ; zp_nonInt_scratchWord1+0: src ptr lo
  ; zp_nonInt_scratchWord1+1: src ptr hi
  ; mainMem_nonInt_scratchWord2+0: dst vram addr lo
  ; mainMem_nonInt_scratchWord2+1: dst vram addr hi
  loadUncompressedGrp:
    ; load banks for target
;    lda mainMem_nonInt_scratchWord2+0.w
    and #$7F
    tam #$04
    ina
    tam #$08
    
    ; $7C-7D = src
    ; TODO: this is now pointless since it's all zero-page memory.
    ; just use nonInt_scratchWord1 as src.
    lda zp_nonInt_scratchWord1+0.b
    sta $7C.b
    lda zp_nonInt_scratchWord1+1.b
    sta $7D.b
    
    ; set vram dst
    st0 #vdp_reg_mawr
    lda mainMem_nonInt_scratchWord2+0.w
    sta vdp_dataLo.w
    lda mainMem_nonInt_scratchWord2+1.w
    sta vdp_dataHi.w
    st0 #vdp_reg_vwr
    
    ; $14-15 = size from src
    ; lo byte
    lda ($7C.b)
    sta $14.b
    inc $7C.b
    bne +
      inc $7D.b
    +:
    ; hi byte
    lda ($7C.b)
    sta $15.b
    inc $7C.b
    bne +
      inc $7D.b
    +:
    
    ; copy data to vdp
    cly
    @copyLoop:
      lda ($7C),Y
      sta vdp_dataLo.w
      iny
      lda ($7C),Y
      sta vdp_dataHi.w
      
      ; decrement remaining words count
      lda $14.b
      dec $14.b
      cmp #$00
      bne +
        lda $15.b
        beq @done
        dec $15.b
      +:
      
      ; increment src
      iny
      bne +
        inc $7D.b
      +:
      
      bra @copyLoop
    @done:
      
;       .rept 2
;         inc $7C.b
;         bne +
;           inc $7D.b
;         +:
;       .endr
    
    rts
.ends

;==================================
; loadHalfTilemapList
;==================================

.bank metabank_bank00
.section "loadHalfTilemapList: uncompressed data check 1" orga $EB1B overwrite
  jmp loadHalfTilemapList_uncmpCheck
.ends

.bank metabank_bank00
.section "loadHalfTilemapList: uncompressed data check 2" free
  loadHalfTilemapList_uncmpCheck:
    ; make up work
    lda ($70.b)
    bne +
      ; done
      jmp $EB2F
    +:
    
    ; check if highest bit of pattern num is set (will not be used legitimately
    ; since it's outside the valid VRAM range).
    ; if so, this is uncompressed data.
    and #$08
    beq +
      ; copy param list
      jsr copyGraphicLoadListParams
      
      ; load uncompressed graphic
      nonInt_trampolineToCodeBankRoutine loadUncompressedHalfTilemap
      
      ; resume loop
      jmp $EB22
    +:
    
    ; handle as normal
    jmp $EB1F
.ends

makeSectionInCodeBank "loadUncompressedHalfTilemap 1"
  ; A = high byte of output with bit 0x08 set
  ; zp_nonInt_scratchWord1+0: src ptr lo
  ; zp_nonInt_scratchWord1+1: src ptr hi
  ; mainMem_nonInt_scratchWord2+0: dst y
  ; mainMem_nonInt_scratchWord2+1: dst x
  loadUncompressedHalfTilemap:
    ; save hi byte
    and #~$08
;    sta zp_nonInt_scratchWord0_lo.w
    sta $19.b
    
    ; load banks for target
    lda #grpBankBaseBank+((:newLoadableTilemaps)-metabank_grpbank_0)
    tam #$04
    ina
    tam #$08
    
    ; $7C-7D = src
    lda zp_nonInt_scratchWord1+0.b
    sta $7C.b
    lda zp_nonInt_scratchWord1+1.b
    sta $7D.b
    
    ; $1E/1F = dst y/x
    lda mainMem_nonInt_scratchWord2+0.w
    sta $1E.b
    lda mainMem_nonInt_scratchWord2+1.w
    sta $1F.b
    
;     ; set vram dst
;     st0 #vdp_reg_mawr
;     lda mainMem_nonInt_scratchWord2+0.w
;     sta vdp_dataLo.w
;     lda mainMem_nonInt_scratchWord2+1.w
;     sta vdp_dataHi.w
;     st0 #vdp_reg_vwr
    
    ; $16/$17 = dst h/w
    ; lo byte
    lda ($7C.b)
    sta $16.b
    inc $7C.b
    bne +
      inc $7D.b
    +:
    ; hi byte
    lda ($7C.b)
    sta $17.b
    inc $7C.b
    bne +
      inc $7D.b
    +:
    
    ; set up output addr
    jsr tilemapPosToAddr
    
    ; copy data to vdp
    ; (mostly copy of code from original routine)
    cly
    @copyLoop:
      ; set MAWR
      st0 #$00
      lda $7A.b
      sta $0002.w
      lda $7B.b
      sta $0003.w
      st0 #$02
      ; $14 = counter for remaining tiles in row
      lda $17.b
      sta $14.b
      ; loop
      -:
        ; fetch next lo byte
        lda ($7C.b),Y
        iny
        
        sta $0002.w
        ; use preset high byte
        lda $19.b
        sta $0003.w
        dec $14.b
        bne -
      ; advance dst to next row
      lda $7A.b
      clc 
      adc #$40
      sta $7A.b
      bcc +
        inc $7B.b
      +:
      ; rebase src from advance in Y?
      tya 
      clc
      adc $7C.b
      sta $7C.b
      lda $7D.b
      bcc +
        inc $7D.b
      +:
      cly 
      ; decrement row count and loop while nonzero
      dec $16.b
      bne @copyLoop
    @done:
    
    rts
.ends

;==================================
; loadFullTilemapList
;==================================

; .bank metabank_bank00
; .section "loadFullTilemapList: uncompressed data check 1" orga $EBB4 overwrite
;   jmp loadFullTilemapList_uncmpCheck
; .ends
; 
; .bank metabank_bank00
; .section "loadFullTilemapList: uncompressed data check 2" free
;   loadFullTilemapList_uncmpCheck:
;     ; make up work
;     lda ($70.b)
;     bne +
;       ; done
;       jmp $EBC8
;     +:
;     
;     ; check if highest bit of pattern num is set (will not be used legitimately
;     ; since it's outside the valid VRAM range).
;     ; if so, this is uncompressed data.
;     and #$08
;     beq +
;       ; copy param list
;       jsr copyGraphicLoadListParams
;       
;       ; load uncompressed graphic
;       nonInt_trampolineToCodeBankRoutine loadUncompressedFullTilemap
;       
;       ; resume loop
;       jmp $EBBB
;     +:
;     
;     ; handle as normal
;     jmp $EBB8
; .ends
; 
; makeSectionInCodeBank "loadUncompressedFullTilemap 1"
;   ; A = high byte of output with bit 0x08 set
;   ; zp_nonInt_scratchWord1+0: src ptr lo
;   ; zp_nonInt_scratchWord1+1: src ptr hi
;   ; mainMem_nonInt_scratchWord2+0: dst y
;   ; mainMem_nonInt_scratchWord2+1: dst x
;   loadUncompressedFullTilemap:
;     ; save hi byte
;     and #~$08
; ;    sta zp_nonInt_scratchWord0_lo.w
;     sta $19.b
;     
;     ; load banks for target
;     lda #grpBankBaseBank+((:newLoadableTilemaps)-metabank_grpbank_0)
;     tam #$04
;     ina
;     tam #$08
;     
;     ; $7C-7D = src
;     lda zp_nonInt_scratchWord1+0.b
;     sta $7C.b
;     lda zp_nonInt_scratchWord1+1.b
;     sta $7D.b
;     
;     ; $1E/1F = dst y/x
;     lda mainMem_nonInt_scratchWord2+0.w
;     sta $1E.b
;     lda mainMem_nonInt_scratchWord2+1.w
;     sta $1F.b
;     
;     ; $16/$17 = dst h/w
;     ; lo byte
;     lda ($7C.b)
;     sta $16.b
;     inc $7C.b
;     bne +
;       inc $7D.b
;     +:
;     ; hi byte
;     lda ($7C.b)
;     sta $17.b
;     inc $7C.b
;     bne +
;       inc $7D.b
;     +:
;     
;     ; set up output addr
;     jsr tilemapPosToAddr
;     
;     ; copy data to vdp
;     ; (mostly copy of code from original routine)
;     cly
;     @copyLoop:
;       ; set MAWR
;       st0 #$00
;       lda $7A.b
;       sta $0002.w
;       lda $7B.b
;       sta $0003.w
;       st0 #$02
;       ; $14 = counter for remaining tiles in row
;       lda $17.b
;       sta $14.b
;       ; loop
;       -:
;         ; fetch next lo byte
;         lda ($7C.b),Y
;         iny
;         sta $0002.w
;         
;         ; add preset hi byte to next input
;         lda ($7C.b),Y
;         iny
;         clc
;         adc $19.b
;         sta $0003.w
;         
;         dec $14.b
;         bne -
;       ; advance dst to next row
;       lda $7A.b
;       clc 
;       adc #$40
;       sta $7A.b
;       bcc +
;         inc $7B.b
;       +:
;       ; rebase src from advance in Y?
;       tya 
;       clc
;       adc $7C.b
;       sta $7C.b
;       lda $7D.b
;       bcc +
;         inc $7D.b
;       +:
;       cly 
;       ; decrement row count and loop while nonzero
;       dec $16.b
;       bne @copyLoop
;     @done:
;     
;     rts
; .ends

;==============================================================================
; graphic replacements
;==============================================================================

;==================================
; macros and sections
;==================================

;=====
; graphic load
;=====

.macro makeGraphicLoadListEntry ARGS bank,ptr,dst
  .db bank
  .dw ptr
  .dw dst
.endm

; .macro makeNewGraphicLoadListEntry ARGS bank,ptr,dst
;   .db bank|$80
;   .dw ptr
;   .dw dst
; .endm

.macro makeNewGraphicLoadListEntry ARGS name,dst
  ; set high bit of bank byte to indicate use of uncompressed data
  .db $80|(grpBankBaseBank+((((:grp_\1)-metabank_grpbank_0)*(metabank_grpbank_0_size/slotSize))))
  .dw grp_\1
  .dw dst
.endm

.macro addLoadableGraphic ARGS name,srcFile
  makeSectionInGrpBank "addLoadableGraphic graphic \1"
    ; first word == number of words to transfer
    grp_\1:
      .dw (grp_\1_end-grp_\1_start)/2
    grp_\1_start:
      .incbin \2
    grp_\1_end:
  .ends
.endm

;=====
; half tilemaps
;=====

; all tilemaps are stored contiguously in a single fixed bank
makeSectionInGrpBank "loadable tilemaps"
  newLoadableTilemaps:
.ends

.macro makeHalfTilemapLoadListEntry ARGS hiByte,ptr,posY,posX
  .db hiByte
  .dw ptr
  .db posY
  .db posX
.endm

.macro makeNewHalfTilemapLoadListEntry ARGS name,hiByte,posY,posX
  .db $08|hiByte
  .dw halfTilemap_\1
  .db posY
  .db posX
.endm

.macro addLoadableHalfTilemap ARGS name,srcFile,height,width
  .section "addLoadableHalfTilemap graphic \1" APPENDTO "loadable tilemaps"
    halfTilemap_\1:
      .db height
      .db width
    halfTilemap_\1_start:
      .incbin \2
    halfTilemap_\1_end:
  .ends
.endm

;=====
; full tilemaps
;=====

.macro makeFullTilemapLoadListEntry ARGS hiByte,ptr,posY,posX
  .db hiByte
  .dw ptr
  .db posY
  .db posX
.endm

.macro makeNewFullTilemapLoadListEntry ARGS name,hiByte,posY,posX
  .db $08|hiByte
  .dw fullTilemap_\1
  .db posY
  .db posX
.endm

.macro addLoadableFullTilemap ARGS name,srcFile,height,width
  .section "addLoadableFullTilemap graphic \1" APPENDTO "loadable tilemaps"
    fullTilemap_\1:
      .db height
      .db width
    fullTilemap_\1_start:
      .incbin \2
    fullTilemap_\1_end:
  .ends
.endm

;==================================
; name entry character selection
;==================================

;=====
; new graphics data
;=====

; makeSectionInGrpBank "new name entry character select grp 1"
;   addLoadableGraphic newNameEntryCharacterSelect,"out/grp/nameentry.bin"
; .ends

addLoadableGraphic newNameEntryCharacterSelect,"out/grp/nameentry.bin"

;=====
; use new graphics
;=====

.bank metabank_gamescript
.section "use new name entry character select 1" free
  gamescr_newNameEntryGraphicLoadList:
    ; original entries
    makeGraphicLoadListEntry $1F,$458F,$1000
    makeGraphicLoadListEntry $2E,$4184,$7000
    ; new entry
    makeNewGraphicLoadListEntry newNameEntryCharacterSelect,$5000
    ; list terminator
    .db $00
.ends

.bank metabank_gamescript
.section "use new name entry character select 2" orga $B3C6 overwrite
  .dw gamescr_newNameEntryGraphicLoadList
.ends

;=====
; new tilemap data
;=====

addLoadableHalfTilemap newNameEntryCharacterSelect,"out/maps/nameentry.bin",17-2,29

;=====
; use new tilemap
;=====

.bank metabank_bank01
.section "use new name entry character select tilemap 1" orga $D4E2 overwrite
  gamescr_newNameEntryTilemapLoadList:
    ; original entries
;    makeHalfTilemapLoadListEntry $C7,$84D1,$03,$02
    ; new entry
    makeNewHalfTilemapLoadListEntry newNameEntryCharacterSelect,$C5,$03+1,$02-$1
    ; list terminator
    .db $00
.ends

; .bank metabank_gamescript
; .section "use new name entry character select 2" orga $B3C6 overwrite
;   .dw gamescr_newNameEntryGraphicLoadList
; .ends

;=====
; modify position of first window
;=====

.bank metabank_gamescript
.section "gamescr: new name entry character select window mod 1" orga $B398 overwrite
;  gamescript_openWindow $02, $02,$01, $13,$1F
  gamescript_openWindow $02, $02+1,$00, $13-2,$20
.ends

;=====
; disable drawing second window
;=====

.bank metabank_gamescript
.section "gamescr: new name entry character select 1" orga $B3A6 overwrite
  .db $00
.ends

;=====
; don't switch pages if select pressed
;=====

; don't include select in the mask of buttons we're waiting to respond to
.bank metabank_gamescript
.section "gamescr: new name entry character select button disable 1" orga $B24E overwrite
;  .db $FF
  .db $FF&(~$04)
.ends

; skip handling of select button if we get to its button check
.bank metabank_gamescript
.section "gamescr: new name entry character select button disable 2" orga $B253 overwrite
  gamescript_nop
  gamescript_nop
  gamescript_nop
  gamescript_nop
.ends

;=====
; limit cursor to new x/y range
;=====

; .define nameEntryXLimit $0E
; .define nameEntryYLimit $09
.define nameEntryXLimit $0C
.define nameEntryYLimit $07

.bank metabank_gamescript
.section "gamescr: new name entry character select wrap behavior 1" orga $B285 overwrite
  .db nameEntryXLimit
.ends

.bank metabank_gamescript
.section "gamescr: new name entry character select wrap behavior 2" orga $B2A1 overwrite
  .db nameEntryXLimit-1
.ends

.bank metabank_gamescript
.section "gamescr: new name entry character select wrap behavior 3" orga $B2AB overwrite
  .db nameEntryYLimit
.ends

.bank metabank_gamescript
.section "gamescr: new name entry character select wrap behavior 4" orga $B2C7 overwrite
  .db nameEntryYLimit-1
.ends

;=====
; new cursor positioning
;=====

; .define nameEntryCursorBaseX $08
; .define nameEntryCursorBaseY $1A
.define nameEntryCursorBaseX $14
.define nameEntryCursorBaseY $2A+8-1

.bank metabank_gamescript
.section "gamescr: new name entry character select cursor pos 1" orga $B42E overwrite
  adc #nameEntryCursorBaseY
.ends

.bank metabank_gamescript
.section "gamescr: new name entry character select cursor pos 2" orga $B456 overwrite
  ; x-pos table
;  .db $0C,$1C,$2C,$3C,$4C,  $64,$74,$84,$94,$A4,  $BC,$CC,$DC,$EC
  .db nameEntryCursorBaseX+$00
  .db nameEntryCursorBaseX+$10
  .db nameEntryCursorBaseX+$20
  .db nameEntryCursorBaseX+$30
  
  .db nameEntryCursorBaseX+$50
  .db nameEntryCursorBaseX+$60
  .db nameEntryCursorBaseX+$70
  .db nameEntryCursorBaseX+$80
  
  .db nameEntryCursorBaseX+$A0
  .db nameEntryCursorBaseX+$B0
  .db nameEntryCursorBaseX+$C0
  .db nameEntryCursorBaseX+$D0
.ends

; don't offset cursor based on page selection bit
.bank metabank_gamescript
.section "gamescr: new name entry character select cursor pos 3" orga $B43C overwrite
  rts
.ends

;=====
; character layout
;=====

.bank metabank_gamescript
.section "gamescr: new name entry character select layout 1" orga $B482 overwrite
  .incbin "out/script/new/name-entry-layout.bin"
.ends

;=====
; special rules to prevent word wrap troubles
;=====

; the word wrap algorithm assumes that words are always separated by exactly
; one space.
; to ensure the user doesn't pick a name that disrupts this, we must ensure that
; - the picked name does not begin or end with any spaces
; - the picked name does not contain multiple consecutive spaces
; - the picked name does not consist of ONLY spaces

; .bank metabank_gamescript
; .section "gamescr: space filtering 1" orga $B16B overwrite
;   .dw nameSpaceFilter
; .ends

.bank metabank_gamescript
.section "gamescr: space filtering 1" orga $B14D overwrite
  gamescript_jump gamescr_nameValidate
.ends

.bank metabank_gamescript
.section "gamescr: space filtering 2" free
  gamescr_nameValidate:
    ; make up work
    gamescript_call $B24D
    gamescript_callNativeCode nameSpaceFilter
    gamescript_jump $B150
  
  ; TODO: i believe hyphens in the name should produce correct wrapping
  ; results regardless of doubling-up, etc., but haven't done anything
  ; to confirm this.
  ; (honestly, i should have just made non-breaking versions of these
  ; characters for use in names and saved myself this hassle)
  nameSpaceFilter:
    ; set up pointer to name
    lda #<$26A4
    sta $DC.b
    lda #>$26A4
    sta $DD.b
    
    cly
    @initialSpaceClear:
      lda ($DC.b),Y
      beq @done
      cmp #namechar_space
      bne +
        iny
        bsr nameSpaceFilter_shiftBack
        dey
        bra @initialSpaceClear
    +:
    
    @checkLoop:
      lda ($DC.b),Y
      beq @done
      
      iny
      cmp #namechar_space
      bne @checkLoop
        lda ($DC.b),Y
        bne +
          ; space followed by terminator = overwrite space with terminator
          dey
          cla
          sta ($DC.b),Y
          bra @done
        +:
        
        cmp #namechar_space
        bne @checkLoop
          ; space followed by space = move remainder of name back one character,
          ; overwriting the second space
          bsr nameSpaceFilter_shiftBack
          dey
          bra @checkLoop
;      @notSpace:
;      bra @checkLoop
    
    @done:
    
;     ; if name consisted of only spaces, and we removed all of them,
;     ; jump back to gamescript and handle as normal "empty name"
;     ; (filling in with default, etc.)
;     lda ($DC.b)
;     bne +
;       jsr jumpToFollowingGamescript
; ;      gamescript_jump $B150
;       gamescript_jump $B17F
;     +:
    
;     ; make up work
;     jmp $D4F5
    rts
    
  nameSpaceFilter_shiftBack:
    phy
      -:
        lda ($DC.b),Y
        dey
        sta ($DC.b),Y
        cmp #$00
        beq @done
        iny
        iny
        bra -
    @done:
    ply
    rts
.ends

;==================================
; magazine graphics
;==================================

;=====
; new graphics data
;=====

addLoadableGraphic newMagazineGrp,"out/grp/magazine.bin"

;=====
; use new graphics
;=====

.bank metabank_bank01
.section "use new magazine graphics 1" orga $CA5A overwrite
  makeNewGraphicLoadListEntry newMagazineGrp,$5000
.ends

; phone book has separate load list
.bank metabank_bank01
.section "use new magazine graphics 2" orga $CB3F overwrite
  makeNewGraphicLoadListEntry newMagazineGrp,$5000
.ends

;=====
; new tilemap data
;=====

addLoadableHalfTilemap newMagazineBg,"out/maps/magazine-bg.bin",16,16
addLoadableHalfTilemap newPhoneBookBg,"out/maps/phonebook-bg.bin",16,16

.macro addHeadlineTilemap ARGS num
  addLoadableHalfTilemap newMagazineHeadline_\1,"out/maps/magazine-headline_\1.bin",2,14
.endm

.rept 7 INDEX count
  addHeadlineTilemap count
.endr

;=====
; use new tilemaps
;=====

.bank metabank_bank01
.section "use new magazine tilemap 1" orga $C668 overwrite
  makeNewHalfTilemapLoadListEntry newMagazineBg,$35,$02,$02
.ends

.bank metabank_bank01
.section "use new magazine tilemap 2" orga $CB80 overwrite
  makeNewHalfTilemapLoadListEntry newPhoneBookBg,$B5,$02,$02
.ends

.macro overwriteHeadlineTilemap ARGS num
  .bank metabank_bank01
  .section "use new magazine headline tilemap \1" orga $C676+((\1)*7) overwrite
    makeNewHalfTilemapLoadListEntry newMagazineHeadline_\1,$35,$05,$03
  .ends
.endm

.rept 7 INDEX count
  overwriteHeadlineTilemap count
.endr

;==================================
; race street graphics
;==================================

;=====
; new graphics data
;=====

addLoadableGraphic newRaceHudStreetGrp,"out/grp/racehud_street.bin"

;=====
; new tilemap data
;=====

addLoadableHalfTilemap newRaceHudStreetMap,"out/maps/racehud-street.bin",4,4

;=====
; use new graphics
;=====

; .bank metabank_gamescript
; .section "use new race street hud 1" free
;   gamescr_newNameEntryGraphicLoadList:
;     ; original entries
;     makeGraphicLoadListEntry $31,$55DC,$6000
; ;    makeGraphicLoadListEntry $32,$4A45,$7000
;     ; new entry
;     makeNewGraphicLoadListEntry newRaceHudStreetGrp,$7000
;     ; list terminator
;     .db $00
; .ends

.bank metabank_bank01
.section "use new race street hud 1" orga $C0D0 overwrite
  makeGraphicLoadListEntry $31,$55DC,$6000
;  makeGraphicLoadListEntry $32,$4A45,$7000
  makeNewGraphicLoadListEntry newRaceHudStreetGrp,$7000
  ; list terminator
  .db $00
.ends

;=====
; use new tilemap
;=====

; .bank metabank_bank01
; .section "use new race street tilemap 1" free
;   newRaceStreetTilemapList:
;     ; original entries
;     makeFullTilemapLoadListEntry $B6,$508C,$00,$00
;     ; new entry
;     makeNewFullTilemapLoadListEntry newRaceHudStreetMap,$00,$02,$01
;     ; list terminator
;     .db $00
; .ends
; 
; .bank metabank_gamescript
; .section "use new race street tilemap 2" orga $ overwrite
;   .dw newRaceStreetTilemapList
; .ends

.bank metabank_bank01
.section "use new race street tilemap 1" free
  newRaceStreetTilemapList:
    ; player's hud
    makeNewHalfTilemapLoadListEntry newRaceHudStreetMap,$C7,$02,$01
    ; opponent (or player 2's) hud
    makeNewHalfTilemapLoadListEntry newRaceHudStreetMap,$C7,$02,$18
    ; list terminator
    .db $00
.ends

.bank metabank_race
.section "use new race street tilemap 2" orga $4399 overwrite
  jsr loadNewRaceStreetTilemaps
  nop
.ends

.bank metabank_race
.section "use new race street tilemap 3" free
  loadNewRaceStreetTilemaps:
    ; load additional tilemap list for hud
    lda #<newRaceStreetTilemapList
    sta $70.b
    lda #>newRaceStreetTilemapList
    sta $71.b
    jsr loadHalfTilemapList
    
    ; make up work
    lda #$C7
    sta $6C.b
    rts
.ends

;=====
; offset transmission gear indicator sprite to compensate for new layout
;=====

.define transmissionGearSpriteXShift 18

.bank metabank_race
.section "transmission gear sprite shift 1" orga $48CE overwrite
  ; player
  .db $08+transmissionGearSpriteXShift
  ; opponent
  .db $C0+transmissionGearSpriteXShift
.ends

; the shifter knob is positioned relative to the offsets modified above,
; so we have to update its position accordingly to keep it in the same spot

; manual
.bank metabank_race
.section "transmission gear sprite shift 2" orga $482B overwrite
  .db $24+(-transmissionGearSpriteXShift)
  .db $1C+(-transmissionGearSpriteXShift)
  .db $1C+(-transmissionGearSpriteXShift)
  .db $1C+(-transmissionGearSpriteXShift)
  .db $24+(-transmissionGearSpriteXShift)
  .db $24+(-transmissionGearSpriteXShift)
  .db $2C+(-transmissionGearSpriteXShift)
  .db $2C+(-transmissionGearSpriteXShift)
  .db $2C+(-transmissionGearSpriteXShift)
.ends

; automatic
.bank metabank_race
.section "transmission gear sprite shift 3" orga $471D overwrite
  adc #$24+(-transmissionGearSpriteXShift)
.ends

;==================================
; race circuit graphics
;==================================

;=====
; new graphics data
;=====

addLoadableGraphic newRaceHudCircuitGrp,"out/grp/racehud_circuit.bin"

;=====
; new tilemap data
;=====

addLoadableHalfTilemap newRaceHudCircuitMap,"out/maps/racehud-circuit.bin",4,4

;=====
; use new graphics
;=====

.bank metabank_bank01
.section "use new race circuit hud 1" orga $C0C4 overwrite
  makeGraphicLoadListEntry $30,$4ADD,$6000
  makeNewGraphicLoadListEntry newRaceHudCircuitGrp,$7000
  ; list terminator
  .db $00
.ends

;=====
; use new tilemap
;=====

.bank metabank_bank01
.section "use new race circuit tilemap 1" free
  newRaceCircuitTilemapList:
    ; player's hud
    makeNewHalfTilemapLoadListEntry newRaceHudCircuitMap,$C7,$02,$01
    ; opponent (or player 2's) hud
    makeNewHalfTilemapLoadListEntry newRaceHudCircuitMap,$C7,$02,$18
    ; list terminator
    .db $00
.ends

.bank metabank_race
.section "use new race circuit tilemap 2" orga $4339 overwrite
  jsr loadNewRaceCircuitTilemaps
  nop
.ends

.bank metabank_race
.section "use new race circuit tilemap 3" free
  loadNewRaceCircuitTilemaps:
    ; load additional tilemap list for hud
    lda #<newRaceCircuitTilemapList
    sta $70.b
    lda #>newRaceCircuitTilemapList
    sta $71.b
    jsr loadHalfTilemapList
    
    ; make up work
    lda #$C7
    sta $6C.b
    rts
.ends

;==================================
; updated race sprite messages
;==================================

; addLoadableGraphic newRaceStreetSpriteOverlayGrp,"out/grp/racesprites_street.bin"
addLoadableGraphic newRaceCircuitSpriteOverlayGrp,"out/grp/racesprites_circuit.bin"

.bank metabank_bank01
.section "new race circuit sprite overlay 1" orga $C0DC overwrite
  makeGraphicLoadListEntry $30,$45CA,$4000
  makeGraphicLoadListEntry $1A,$53D7,$4800
  makeNewGraphicLoadListEntry newRaceCircuitSpriteOverlayGrp,$5800
.ends

;==================================
; credits
;==================================

;=====
; new graphics data
;=====

addLoadableGraphic newCreditsSprites1,"out/grp/credits_sprites_grpchunk1.bin"
addLoadableGraphic newCreditsSprites2,"out/grp/credits_sprites_grpchunk2.bin"

;=====
; use new graphics data
;=====

.bank metabank_bank01
.section "use new credits sprites 1" orga $CC65 overwrite
  makeGraphicLoadListEntry $33,$4D14,$1000
  makeNewGraphicLoadListEntry newCreditsSprites1,$2000
  makeNewGraphicLoadListEntry newCreditsSprites2,$3800
  ; rest of list remains as-is
.ends

;=====
; new sprite definition data
;=====

.macro addCreditsSpriteDef ARGS num
  .section "new credits sprite definitions \1" APPENDTO "new sprite definitions"
    creditsSpriteDef_\1:
      .incbin "out/grp/credits_sprites_\1_conv.bin"
  .ends
.endm

.rept 37 INDEX count
  addCreditsSpriteDef count
.endr

.section "new credits empty sprite definition" APPENDTO "new sprite definitions"
  creditsSpriteDef_empty:
    .db $00
.ends

.macro replaceSpriteObjDef ARGS objId,newDefPtr
  .bank metabank_sprites
  .section "sprite obj \1 definition replacement" orga $4000+((\1)*2) OVERWRITE
    .dw newDefPtr
  .ends
.endm

replaceSpriteObjDef $243,creditsSpriteDef_0
replaceSpriteObjDef $241,creditsSpriteDef_1
replaceSpriteObjDef $248,creditsSpriteDef_2
replaceSpriteObjDef $255,creditsSpriteDef_4
replaceSpriteObjDef $23C,creditsSpriteDef_5
replaceSpriteObjDef $25B,creditsSpriteDef_6
replaceSpriteObjDef $24E,creditsSpriteDef_7
replaceSpriteObjDef $258,creditsSpriteDef_8
replaceSpriteObjDef $23A,creditsSpriteDef_9
replaceSpriteObjDef $25C,creditsSpriteDef_10
replaceSpriteObjDef $257,creditsSpriteDef_11
replaceSpriteObjDef $237,creditsSpriteDef_12
replaceSpriteObjDef $245,creditsSpriteDef_13
replaceSpriteObjDef $253,creditsSpriteDef_14
replaceSpriteObjDef $25A,creditsSpriteDef_15
replaceSpriteObjDef $247,creditsSpriteDef_16
replaceSpriteObjDef $251,creditsSpriteDef_17
replaceSpriteObjDef $250,creditsSpriteDef_18
replaceSpriteObjDef $238,creditsSpriteDef_19
replaceSpriteObjDef $23B,creditsSpriteDef_20
replaceSpriteObjDef $239,creditsSpriteDef_21
replaceSpriteObjDef $24D,creditsSpriteDef_22
replaceSpriteObjDef $23E,creditsSpriteDef_23
replaceSpriteObjDef $249,creditsSpriteDef_24
replaceSpriteObjDef $259,creditsSpriteDef_25
replaceSpriteObjDef $252,creditsSpriteDef_26
replaceSpriteObjDef $24F,creditsSpriteDef_27
replaceSpriteObjDef $24A,creditsSpriteDef_28
replaceSpriteObjDef $23D,creditsSpriteDef_29
replaceSpriteObjDef $242,creditsSpriteDef_30
replaceSpriteObjDef $254,creditsSpriteDef_32
replaceSpriteObjDef $24B,creditsSpriteDef_33
replaceSpriteObjDef $244,creditsSpriteDef_34
replaceSpriteObjDef $256,creditsSpriteDef_35
replaceSpriteObjDef $24C,creditsSpriteDef_36

; generic components we no longer need or want to be displayed
replaceSpriteObjDef $23F,creditsSpriteDef_empty
replaceSpriteObjDef $240,creditsSpriteDef_empty
replaceSpriteObjDef $246,creditsSpriteDef_empty

;==================================
; title logo
;==================================

;=====
; new graphics data
;=====

addLoadableGraphic newTitleLogo,"out/rsrc_raw/grp/title_logo.bin"

;=====
; use new graphics data
;=====

; title screen
.bank metabank_bank01
.section "use new title logo 1" orga $CA25 overwrite
  makeNewGraphicLoadListEntry newTitleLogo,$2000
.ends

; end of intro
.bank metabank_bank01
.section "use new title logo 2" orga $C93F overwrite
  makeNewGraphicLoadListEntry newTitleLogo,$2000
.ends

;==================================
; when giving player a password,
; display it in monospace font
;==================================

;=====
; start
;=====

.bank metabank_gamescript
.section "gamescr: password monospace start 1" orga $5E90 overwrite
  gamescript_jump gamescr_passwordMonospace
.ends

.bank metabank_gamescript
.section "gamescr: password monospace start 2" free
  gamescr_passwordMonospace:
    ; set window 3 font = monospace
    gamescript_writeByte printing_currentFont_array+3,fontId_monospace
    ; make up work
    gamescript_runText $03,$64FF
    gamescript_jump $5E94
.ends

;=====
; end
;=====

.bank metabank_gamescript
.section "gamescr: password monospace end 1" orga $5EE2 overwrite
  gamescript_jump gamescr_passwordMonospaceEnd
.ends

.bank metabank_gamescript
.section "gamescr: password monospace end 2" free
  gamescr_passwordMonospaceEnd:
    ; make up work
    gamescript_runText $03,$651A
    gamescript_waitForTextDone
    ; set window 3 font = std
    gamescript_writeByte printing_currentFont_array+3,fontId_std
    gamescript_jump $5EE7
.ends

;==================================
; print user-entered names and
; passwords in monospace font
;==================================

.bank metabank_gamescript
.section "gamescr: typed password monospace start 1" orga $B32D overwrite
  gamescript_jump gamescr_typedPasswordMonospace
.ends

.bank metabank_gamescript
.section "gamescr: typed password monospace start 2" free
  gamescr_typedPasswordMonospace:
    ; set window 0 font = monospace
    gamescript_writeByte printing_currentFont_array+0,fontId_monospace
    
    ; make up work
    gamescript_runText $00,$27A4
    gamescript_waitForTextDone
    
    ; reset to std font
    gamescript_writeByte printing_currentFont_array+0,fontId_std
    
    gamescript_jump $B332
.ends

;==============================================================================
; new textscripts for translation
;==============================================================================

.macro replaceGamescriptTextPtrByRawOffset ARGS offset,newPtr
  .bank metabank_gamescript
  .section "gamescript: text ptr replacement \1" orga $4000+offset overwrite
    .dw newPtr
  .ends
  
;   .print $4000+offset,"\n"
.endm

;==================================
; replace recycled clear command
;==================================

; the data at 0x2C80 is called as an independent textscript.
; however, it's actually the last 2 bytes of a 4-byte script at 0x2C7E.
; because we need 3 bytes for the jump commands that are placed at the original
; position of each textscript, we can't insert a jump command for this:
; not only would it overlap the jump command for 0x2C7E, 2 bytes isn't long
; enough for a jump in the first place.
; so we instead have to create a special replacement and manually update all
; references to it.

makeSemiSuperfreeSection "maintext recycled clear replacement 1",metabank_maintext
  textscript_recycledClearReplacement:
    .incbin "out/script/new/recycled-clear-replacement.bin"
.ends

; replace references
replaceGamescriptTextPtrByRawOffset $07B5,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $08E9,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $0A67,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $0F50,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $0FB1,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $11FA,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $1202,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $1284,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $13A3,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $13AB,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $1403,textscript_recycledClearReplacement
; probably not a real reference
;replaceGamescriptTextPtrByRawOffset $183F,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $1C2E,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $1C76,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $1C9B,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $1CE3,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $3172,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $31C7,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $34AA,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $3723,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $4766,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $47AF,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $47D1,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $4813,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $4837,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $486C,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $48B6,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $48E3,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $49B0,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $4AF8,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $4B18,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $4B57,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $4C56,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5135,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5162,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5188,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $51AE,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $51E0,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $52B8,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $52E9,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $54DE,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5500,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5510,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5555,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $55C6,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $55D6,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5686,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $56A5,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5762,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $5FE1,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $6001,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $601E,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $6427,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $645E,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $66A6,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $66CA,textscript_recycledClearReplacement
replaceGamescriptTextPtrByRawOffset $6700,textscript_recycledClearReplacement

;==================================
; replace backup memory strings
;==================================

; .macro addBackupMemString ARGS id,orig
;   makeSemiSuperfreeSection "backup data string \1",metabank_newtext,metabank_maintext
;     backupStr_\1:
;       .incbin "out/script/bank01/backup-\1.bin"
;   .ends
;   
;   .section "backup data string overwrite \1" BANK metabank_bank01 SLOT 0 ORGA \2 FORCE
;     text_jumpToPossiblyNewLoc backupStr_\1
;   .ends
; .endm
; 
; addBackupMemString 0,$D740
; addBackupMemString 1,$D753
; addBackupMemString 2,$D767
; addBackupMemString 3,$D78A

.macro addBackupMemString ARGS id
  ; must go in maintext so this can be reached via gamescript call
  makeSemiSuperfreeSection "backup data string \1",metabank_maintext
    backupStr_\1:
      .incbin "out/script/bank01/backup-\1.bin"
  .ends
.endm

.macro overwriteBackupMemStringRef ARGS id,addr,slt
  .section "backup data string overwrite \1 \@ 1" BANK metabank_bank01 SLOT 0 ORGA \2 OVERWRITE
    gamescript_jump backupStrRef_\1_\@
  .ends
  
  .section "backup data string overwrite \1 \@ 2" BANK metabank_gamescript SLOT 0 FREE
    backupStrRef_\1_\@:
      gamescript_runText \3,backupStr_\1
      ; jump back to next script command after the one we replaced
      gamescript_jump addr+4
  .ends
.endm

addBackupMemString 0
addBackupMemString 1
addBackupMemString 2
addBackupMemString 3

overwriteBackupMemStringRef 1,$D65D,3
overwriteBackupMemStringRef 2,$D6AE,3
overwriteBackupMemStringRef 3,$D6E5,0
overwriteBackupMemStringRef 0,$D719,0
overwriteBackupMemStringRef 2,$D729,0

;==============================================================================
; new special names/passwords
;==============================================================================

.macro makeSpecialNameEntry ARGS num
  specialName_\1:
    .incbin "out/script/specialname/specialname-\1.bin"
.endm

.macro addSpecialNameEntryRef ARGS num
  .dw specialName_\1
.endm

.bank metabank_gamescript
.section "gamescr: special name list 1" orga $B53E overwrite
  .rept 32 INDEX count
    addSpecialNameEntryRef count
  .endr
.ends

.bank metabank_gamescript
.section "gamescr: special name list 2" free
  .rept 32 INDEX count
    makeSpecialNameEntry count
  .endr
.ends

;==============================================================================
; window size adjustments
;==============================================================================

;==================================
; title menu
;==================================

.bank metabank_gamescript
.section "window size adjustment: title menu 1" orga $6AF2 overwrite
;  gamescript_openWindow $00, $11,$07,  $0B,$12
  gamescript_openWindow $00, $11,$07+$1,  $0B,$12-$2
.ends

;==================================
; title menu: continue
;==================================

.bank metabank_gamescript
.section "window size adjustment: title menu continue 1" orga $6B29 overwrite
  gamescript_openWindow $00, $10,$09+2,  $07,$0E-4
.ends

;==================================
; vs computer: main menu
;==================================

.bank metabank_gamescript
.section "window size adjustment: vs computer main menu 1" orga $BAB8 overwrite
;  gamescript_openWindow $00, $11,$07,  $0B,$12
  gamescript_openWindow $01, $07,$03, $12-3,$12
.ends

;==============================================================================
; lip flap algorithm adjustments
;==============================================================================

.bank metabank_bank00
.section "lip flaps 1" orga $FD9A overwrite
  jmp doNewLipFlaps
.ends

.section "lip flaps 2" APPENDTO "new textscript area"
  ; A = ID of last character printed
  ; Y = offset in array containing (probably) frame IDs for lip flaps
  doNewLipFlaps:
    ; use bit 0x02 of last char ID, inverted, to determine flap state,
    ; rather than using 0x01 as-is. this effectively results in half as many
    ; lip movements, compensating for the global doubling of text speed.
    ; inverting the value is necessary because terminal punctuation characters
    ; are assigned to codepoints that have bit 0x02 set, and we want to
    ; ensure that if a delay is present at the end of a box (as in the ending),
    ; the lip flaps are closed for the duration.
    
    ; original logic
;     and #$01
;     clc
;     adc $2492.w,Y
;     sta $2412.w,Y
    
    ; special case: if 0x00 (terminator), always close lip flaps
    and #$FF
    beq +
      and #$02
      eor #$02
      lsr
    +:
    clc
    adc $2492.w,Y
    sta $2412.w,Y
    rts
.ends

;==============================================================================
; fix bad race init after draw
;==============================================================================

; if a race ends in a draw, the game restarts it from scratch, looping until
; there is a winner.
; when doing so, it mistakenly leaves the vsync and line interrupt handlers
; for race mode enabled as it runs the race setup code.
; this was clearly not intended to happen, as the line interrupt handler
; hijacks the VDP select register while the race graphics are being loaded,
; causing writes intended for VRAM to go to an unrelated register instead.

; the original game gets away with this because it doesn't actually *need* to
; load those graphics at all -- they're already loaded, and the register that
; gets corrupted is restored afterward. but in the translation, our extensions
; to the graphics load routine load in an alternate page on top of the code
; containing the custom interrupt handlers, so if an irq1 interrupt occurs while
; the new code is running, the game jumps to some random code and dies.

; so, we restore the default line and vsync interrupt handlers at the start of
; the race init code, ensuring this issue doesn't happen. the handlers are
; ultimately updated to their intended values later in init, so this is fine.

.bank metabank_race
.section "disable line interrupt during race init 1" orga $4000 overwrite
  jsr disableRaceLineInt
.ends

.bank metabank_race
.section "disable line interrupt during race init 2" free
  disableRaceLineInt:
    ; FIXME? the original game never protects modifications to its interrupt
    ; handlers, so this may be unnecessary. perhaps it just relies on never
    ; lagging.
    sei
      ; reset line interrupt handler to default
      lda #$E7
      sta $48.b
      sta $4A.b
      lda #$E2
      sta $49.b
      sta $4B.b
    cli
    
    ; make up work
    stz $2220.w
    rts
.ends
