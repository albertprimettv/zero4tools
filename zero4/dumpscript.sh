set -o errexit

make libpce && make zero4_scriptgen
./zero4_scriptgen
