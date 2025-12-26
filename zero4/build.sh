#!/usr/bin/env bash

echo "*******************************************************************************"
echo "Updating build information..."
echo "*******************************************************************************"

buildStartTime=$(date)

# build counter for tracking just how many builds this goes through
if [ ! -e "build_counter.txt" ]; then
  printf "0" > "build_counter.txt"
fi

lastBuildNum=$(cat "build_counter.txt")
currentBuildNum=$((lastBuildNum + 1))
printf "$currentBuildNum" > "build_counter.txt"


if [ ! -e "build_log.txt" ]; then
  > "build_log.txt"
else
  printf "\\n\\n" >> "build_log.txt"
fi
printf "Build $currentBuildNum\\n  Started:   $buildStartTime" >> "build_log.txt"

echo "*******************************************************************************"
echo "Setting up environment..."
echo "*******************************************************************************"

LOCALEID=$1

if [ "$1" = "" ]; then
#   echo "Usage: build.sh <region>"
#   echo "Regions: en"
# #   echo "Platforms: native"
#   exit 0;
  LOCALEID="en"
fi

# LOCALEID=$1
# platformName=$2

# if [ "$platformName" = "" ]; then
#   platformName="native"
# fi

set -o errexit

BASE_PWD=$PWD
PATH=".:$PATH"
INROM="zero4.pce"
OUTROM="zero4_build.pce"
WLADX="./wla-dx/build/binaries/wla-huc6280"
WLALINK="./wla-dx/build/binaries/wlalink"

function remapPalette() {
  oldFile=$1
  palFile=$2
  newFile=$3
  
  convert "$oldFile" -dither None -remap "$palFile" PNG32:$newFile
}

function remapPaletteOverwrite() {
  newFile=$1
  palFile=$2
  
  remapPalette $newFile $palFile $newFile
}

mkdir -p out

echo "********************************************************************************"
echo "Building project tools..."
echo "********************************************************************************"

make -j `nproc` blackt
make -j `nproc` libpce
make -j `nproc`

if [ ! -f $WLADX ]; then
  
  echo "********************************************************************************"
  echo "Building WLA-DX..."
  echo "********************************************************************************"
  
  cd wla-dx
#    cmake -G "Unix Makefiles" .
    mkdir -p build && cd build && cmake ..
    make -j `nproc`
  cd $BASE_PWD
  
fi

echo "*******************************************************************************"
echo "Copying base ROM..."
echo "*******************************************************************************"

cp $INROM $OUTROM

echo "*******************************************************************************"
echo "Copying game files..."
echo "*******************************************************************************"

rm -rf "out/rsrc"
cp -r "rsrc/${LOCALEID}" "out/rsrc"
cp -r "rsrc_raw" "out"

echo "*******************************************************************************"
echo "Building font..."
echo "*******************************************************************************"

mkdir -p "out/font"

zero4_fontbuild "out/font/" "${LOCALEID}"

echo "*******************************************************************************"
echo "Building script..."
echo "*******************************************************************************"

mkdir -p out/scripttxt
mkdir -p out/scriptwrap
mkdir -p out/script

zero4_scriptimport "$LOCALEID"

zero4_scriptwrap "out/scripttxt/spec.txt" "out/scriptwrap/spec.txt" "en"

zero4_scriptbuild "out/scriptwrap/" "out/script/" "$LOCALEID"

echo "*******************************************************************************"
echo "Building graphics..."
echo "*******************************************************************************"

mkdir -p "out/grp"
mkdir -p "out/maps"

tempFontFile="out/.fontrender_temp"
tempFontImg="out/.fontrender_temp_img.png"

function renderString() {
  fontPath=$1
  outFile=$2
  text=$3
  
  printf "$text" > $tempFontFile
  
  zero4_fontrender "$fontPath/" "$tempFontFile" "$fontPath/table.tbl" "$outFile"
  
  # TEST
#   convert "$1.png"\
#     \( -size 1000x1000 xc:black \) -geometry +0+0 -compose DstOver -composite\
#     PNG32:$1.png
}

function renderStringAt() {
  fontPath=$1
  outFile=$2
  xPos=$3
  yPos=$4
  text=$5
  
  renderString $fontPath $tempFontImg "$text"
  
  fontPath=$1
  outFile=$2
  xPos=$3
  yPos=$4
  text=$5
  
  convert $outFile\
    \( $tempFontImg \) -geometry +$xPos+$yPos -compose Over -composite\
    PNG32:$outFile
}

function renderPremadeString() {
  fontPath=$1
  unmappingTable=$2
  outFile=$3
  src=$4
  
#  echo $src
  
  zero4_fontrender "$fontPath/" "$src" "$fontPath/table.tbl" "$outFile" --unmap "$unmappingTable"
}

function renderPremadeStringAt() {
  fontPath=$1
  unmappingTable=$2
  outFile=$3
  xPos=$4
  yPos=$5
  src=$6
  
  renderPremadeString $fontPath "$unmappingTable" $tempFontImg "$src"
  
  fontPath=$1
  unmappingTable=$2
  outFile=$3
  xPos=$4
  yPos=$5
  src=$6
  
  convert $outFile\
    \( $tempFontImg \) -geometry +$xPos+$yPos -compose Over -composite\
    PNG32:$outFile
}

nameEntry_baseX=8
nameEntry_baseY=8
nameEntry_spacingX=0
nameEntry_spacingY=16
nameEntry_numCols=1
nameEntry_numRows=7

currentNameEntryEntry=0

function renderPremadeNameEntryString() {
  dst=$1
  unmappingTable=$2
  num=$3
  src=$4
  
  currentNameEntryEntry=$(($currentNameEntryEntry + 1))
  
  renderPremadeString "font/${LOCALEID}/monospace" "$unmappingTable" $tempFontImg "$src"
  
  row=$(($num / $nameEntry_numCols))
  col=$(($num % $nameEntry_numCols))
  
  xPos=$(( $nameEntry_baseX + ($col * $nameEntry_spacingX) ))
  yPos=$(( $nameEntry_baseY + ($row * $nameEntry_spacingY) ))
  
#   echo $xPos $yPos
  
  convert $dst\
    \( $tempFontImg \) -geometry +$xPos+$yPos -compose Over -composite\
    PNG32:$dst
}

renderPremadeNameEntryString "out/rsrc/nameentry.png" "table/${LOCALEID}/zero4.tbl" 0 "out/script/new/name-entry-new_0.bin"
renderPremadeNameEntryString "out/rsrc/nameentry.png" "table/${LOCALEID}/zero4.tbl" 1 "out/script/new/name-entry-new_1.bin"
renderPremadeNameEntryString "out/rsrc/nameentry.png" "table/${LOCALEID}/zero4.tbl" 2 "out/script/new/name-entry-new_2.bin"
renderPremadeNameEntryString "out/rsrc/nameentry.png" "table/${LOCALEID}/zero4.tbl" 3 "out/script/new/name-entry-new_3.bin"
renderPremadeNameEntryString "out/rsrc/nameentry.png" "table/${LOCALEID}/zero4.tbl" 4 "out/script/new/name-entry-new_4.bin"
renderPremadeNameEntryString "out/rsrc/nameentry.png" "table/${LOCALEID}/zero4.tbl" 5 "out/script/new/name-entry-new_5.bin"
renderPremadeNameEntryString "out/rsrc/nameentry.png" "table/${LOCALEID}/zero4.tbl" 6 "out/script/new/name-entry-new_6.bin"

# recolor font to match foreground text color
recolor "out/rsrc/nameentry.png" "out/rsrc/nameentry.png"\
  -c FFFFFF FCFC90

#=====
# magazine headlines
#=====

if [ ${LOCALEID} = "en" ]; then
  for i in `seq 0 6`; do
    cp "out/rsrc/magazine-headline_clean.png" "out/rsrc/magazine-headline_$i.png"
  done
  
  # I'm justifying not putting these in the script itself as "they're supposed to
  # be art assets and I'm just using a font here as a shortcut"
  
  # キング、四天王安たい！
#   renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_0.png" 0 0\
#     " KING AND BIG "
#   renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_0.png" 0 8\
#     "  FOUR REIGN! "
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_0.png" 0 0\
    "KING, BIG FOUR"
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_0.png" 0 8\
    " STILL REIGN! "
  
  # 速すぎる、キング！
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_1.png" 0 0\
    "  THE {KING}  "
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_1.png" 0 8\
    "   OF SPEED!  "
  
  # キングまたしても圧勝
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_2.png" 0 0\
    " KING CRUSHES "
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_2.png" 0 8\
    "'EM ALL AGAIN!"
  
  # またしても、キング！
#   renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_3.png" 0 0\
#     " KING'S STILL "
#   renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_3.png" 0 8\
#     "    AT IT!    "
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_3.png" 0 0\
    "  KING STILL  "
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_3.png" 0 8\
    "CAN'T BE BEAT!"
  
  # キング、あやうし！！
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_4.png" 0 0\
    "  COULD KING  "
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_4.png" 0 8\
    " BE USURPED!? "
  
  # 四天王、あやうし！！
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_5.png" 0 0\
    "   WATCH OUT, "
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_5.png" 0 8\
    "   BIG FOUR!  "
  
  # 新入現る！！
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_6.png" 0 0\
    "A NEW CHALLEN-"
  renderStringAt "font/${LOCALEID}/headline/" "out/rsrc/magazine-headline_6.png" 0 8\
    "GER APPROACHES"
fi

#=====
# credits
#=====

function renderCreditsString() {
  dst=$1
  num=$2
  
  renderPremadeString "font/${LOCALEID}/credits" "table/${LOCALEID}/zero4.tbl" $tempFontImg "out/script/credits/credits-${num}.bin"
  
  convert \( -size 256x16 xc:transparent \)\
    \( $tempFontImg \) -geometry +0+0 -compose Over -composite\
    PNG32:$dst
}

for i in `seq 0 36`; do
  renderCreditsString "out/rsrc/credits_${i}.png" $i
done

spritebuild_pce "" "out/rsrc/credits.txt" "rsrc_raw/pal/credits_sprite.pal" "out/grp/credits_sprites.bin"
# split into two chunks for loading
datsnip "out/grp/credits_sprites.bin" 0x0 0x3000 "out/grp/credits_sprites_grpchunk1.bin"
datsnip "out/grp/credits_sprites.bin" 0x3000 0x3000 "out/grp/credits_sprites_grpchunk2.bin"

for i in `seq 0 36`; do
  zero4_spriteconv "out/grp/credits_sprites_${i}.bin" "out/grp/credits_sprites_${i}_conv.bin"
done

#=====
# title logo
#=====

spriteundmp_pce "out/rsrc/title_logo.png" "out/rsrc/title_logo.bin" -p "rsrc_raw/pal/title_sprite_logo_line.pal"

datpatch "out/rsrc_raw/grp/title_logo.bin" "out/rsrc_raw/grp/title_logo.bin" "out/rsrc/title_logo.bin" $((0x0A*0x80)) $((48*0x80)) $((2*0x80))
datpatch "out/rsrc_raw/grp/title_logo.bin" "out/rsrc_raw/grp/title_logo.bin" "out/rsrc/title_logo.bin" $((0x1A*0x80)) $((50*0x80)) $((2*0x80))

#datpatch "out/rsrc_raw/grp/title_logo.bin" "out/rsrc_raw/grp/title_logo.bin" "out/rsrc/title_logo.bin" $((0x0C*0x80)) $((52*0x80)) $((1*0x80))
convert "out/rsrc/title_logo.png" -crop 16x16+64+88 +repage PNG32:"out/rsrc/title_logo_extra1.png"
convert "out/rsrc/title_logo.png" -crop 16x16+64+104 +repage PNG32:"out/rsrc/title_logo_extra2.png"
spriteundmp_pce "out/rsrc/title_logo_extra1.png" "out/rsrc/title_logo_extra1.bin" -p "rsrc_raw/pal/title_sprite_logo_line.pal"
spriteundmp_pce "out/rsrc/title_logo_extra2.png" "out/rsrc/title_logo_extra2.bin" -p "rsrc_raw/pal/title_sprite_logo_line.pal"

datpatch "out/rsrc_raw/grp/title_logo.bin" "out/rsrc_raw/grp/title_logo.bin" "out/rsrc/title_logo_extra1.bin" $((0x0C*0x80)) $((0*0x80)) $((1*0x80))
datpatch "out/rsrc_raw/grp/title_logo.bin" "out/rsrc_raw/grp/title_logo.bin" "out/rsrc/title_logo_extra2.bin" $((0x0E*0x80)) $((0*0x80)) $((1*0x80))

#=====
# tilemappers
#=====

for file in tilemappers/*.txt; do
  echo $file
  tilemapper_pce "$file"
done

datsnip "out/grp/race_street.bin" $((0x700*0x20)) $((0xD7*0x20)) "out/grp/racehud_street.bin"
datsnip "out/grp/race_circuit.bin" $((0x700*0x20)) $((0xE2*0x20)) "out/grp/racehud_circuit.bin"

#=====
# "flying"
#=====

function patchFlying() {
  outFile=$1
  
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x0C*0x80)) $((0*0x80)) 0x80
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x0D*0x80)) $((1*0x80)) 0x80
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x1C*0x80)) $((2*0x80)) 0x80
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x1D*0x80)) $((3*0x80)) 0x80
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x0E*0x80)) $((4*0x80)) 0x80
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x0F*0x80)) $((5*0x80)) 0x80
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x1E*0x80)) $((6*0x80)) 0x80
  datpatch "$outFile" "$outFile" "out/rsrc/flying.bin" $((0x1F*0x80)) $((7*0x80)) 0x80
}

# datsnip "out/grp/race_street.bin" $((0x120*0x80)) $((0x80*0x80)) "out/grp/racesprites_street.bin"
datsnip "out/grp/race_circuit.bin" $((0x160*0x80)) $((0x20*0x80)) "out/grp/racesprites_circuit.bin"

spriteundmp_pce "out/rsrc/flying.png" "out/rsrc/flying.bin" -p "rsrc_raw/pal/race_flying_line.pal"

# patchFlying "out/grp/racesprites_street.bin"
patchFlying "out/grp/racesprites_circuit.bin"

echo "*******************************************************************************"
echo "Generating includes..."
echo "*******************************************************************************"

zero4_symgen "$LOCALEID"

echo "********************************************************************************"
echo "Applying ASM patches..."
echo "********************************************************************************"

function applyAsmPatch() {
  infile=$1
  asmname=$2
  prelinked=$3
  
  if [ "$prelinked" = "" ]; then
    prelinked=0
  fi
  
  infile_base=$(basename $infile)
  infile_base_noext=$(basename $infile .bin)
  
  linkfile=${asmname}_link
  
  echo "******************************"
  echo "patching: $asmname"
  echo "******************************"
  
  # generate linkfile
  printf "[objects]\n${asmname}.o" >"asm/$linkfile"
  
  cp "$infile" "asm/$infile_base"
  
  cd asm
    # apply hacks
    ../$WLADX -k -I ".." -D ROMNAME_BASE="${infile_base_noext}" -D ROMNAME="${infile_base}" -D LOCALEID="${LOCALEID}" -D ROMNAME_GEN_INC="gen/${infile_base_noext}.inc" -D PRELINKED=${prelinked} -o "$asmname.o" "$asmname.s"
    ../$WLALINK -v -S "$linkfile" "${infile_base}_build"
  cd $BASE_PWD
  
#  mv -f "asm/${infile_base}_build" "out/base/${infile_base}"
  mv -f "asm/${infile_base}_build" "$infile"
  rm "asm/${infile_base}"
  
  rm asm/*.o
  
  # delete linkfile
  rm "asm/$linkfile"
}

metabanker_pce merge "rsrc_raw/rom_metabanks.txt" "out/metabanks.bin"

#applyAsmPatch $OUTROM "main"
applyAsmPatch "out/metabanks.bin" "main"

metabanker_pce split "rsrc_raw/rom_metabanks.txt" "out/metabanks.bin"

echo "*******************************************************************************"
echo "Success!"
echo "Output file:" $OUTROM
echo "*******************************************************************************"

buildEndTime=$(date)
printf "\\n  Completed: $buildEndTime" >> "build_log.txt"
