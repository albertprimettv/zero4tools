
set -o errexit

tempFontFile=".fontrender_temp"


function outlineSolidPixels() {
#  convert "$1" \( +clone -channel A -morphology EdgeOut Diamond:1 -negate -threshold 15% -negate +channel +level-colors \#000024 \) -compose DstOver -composite "$2"
  convert "$1" \( +clone -channel A -morphology EdgeOut Square:1 -negate -threshold 15% -negate +channel +level-colors \#000024 \) -compose DstOver -composite "$2"
}

function renderString() {
  printf "$2" > $tempFontFile
  
#  ./fontrender "font/12px_outline/" "$tempFontFile" "font/12px_outline/table.tbl" "$1.png"
#  ./fontrender "font/" "$tempFontFile" "font/table.tbl" "$1.png"
  ./fontrender "font/en/italic/" "$tempFontFile" "font/en/italic/table.tbl" "$1.png"
  
  # TEST
#   convert "$1.png"\
#     \( -size 1000x1000 xc:black \) -geometry +0+0 -compose DstOver -composite\
#     PNG32:$1.png
  convert "$1.png"\
    \( -size 1000x1000 xc:gray \) -geometry +0+0 -compose DstOver -composite\
    PNG32:$1.png
}

function renderStringMenulabel() {
  printf "$2" > $tempFontFile
  
#  ./fontrender "font/12px_outline/" "$tempFontFile" "font/12px_outline/table.tbl" "$1.png"
#  ./fontrender "font/" "$tempFontFile" "font/table.tbl" "$1.png"
  ./fontrender "font/menulabel/" "$tempFontFile" "font/menulabel/table.tbl" "$1.png"
  
  # TEST
#   convert "$1.png"\
#     \( -size 1000x1000 xc:black \) -geometry +0+0 -compose DstOver -composite\
#     PNG32:$1.png
}

function renderStringNarrow() {
  printf "$2" > $tempFontFile
  
#  ./fontrender "font/12px_outline/" "$tempFontFile" "font/12px_outline/table.tbl" "$1.png"
#  ./fontrender "font/" "$tempFontFile" "font/table.tbl" "$1.png"
  ./fontrender "font/12px/" "$tempFontFile" "font/12px/table.tbl" "$1.png"
  outlineSolidPixels "$1.png" "$1.png"
}

function renderStringScene() {
  printf "$2" > $tempFontFile
  
#  ./fontrender "font/12px_outline/" "$tempFontFile" "font/12px_outline/table.tbl" "$1.png"
#  ./fontrender "font/" "$tempFontFile" "font/table.tbl" "$1.png"

  ./fontrender "font/scene/" "$tempFontFile" "font/scene/table.tbl" "$1.png"
}



make blackt && make fontrender

renderString render_en1 "click disc brakes drum brakes four-cycle engine bore stroke gross net differential gear FR FF MR 4WD Gan? you ding-dong Prrrr Ka-chunk! own know Restaurant Champ have You ka-chunk sob fun that!? special exactly is this puff-puff being Your I'm beep boop really Gracias geta Ensuto baaaaaw What? steering thought Tres bien And"
renderString render_en2 "Oh, seriously? And who was all full of herself over a few compliments just a minute ago?"
renderString render_en3 "Maybe this girl has a thing for me? All right, let's put the shmooze on..."
renderString render_en4 "Hmph. There's something fishy about this guy."
renderString render_en5 "Okay, now's my chance! I've got to put on music to set the mood and bring this to a climax!"
renderString render_en6 "Which one should I pick?"
renderString render_en7 "That's not gonna work."
renderString render_en8 "Who's that, Imamura? Quit buttin' in, dude!"
renderString render_en9 "Geez. Not the social type, apparently."
renderString render_en10 "Huh? King and Gan know each other?"

rm -f $tempFontFile
