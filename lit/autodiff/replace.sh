
ORIGINAL="-d autodiff"
TARGET="-d autodiff -d ad_func"
FORMAT=".thorin"
FILES=`find . -name "*.thorin"`

for f in $FILES; do
    echo "Replacing $f"
    sed -i "s/$ORIGINAL/$TARGET/g" $f
done
