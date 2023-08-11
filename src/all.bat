make bin/undionly.kkpxe
make bin/undionly.kpxe 
make bin-x86_64-efi/snponly.efi

mkdir output
cp -f bin/undionly.kkpxe output/undionly.kkpxe
cp -f bin/undionly.kpxe output/undionly.kpxe
cp -f bin-x86_64-efi/snponly.efi output/snponly.efi

