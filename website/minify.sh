rm -rf minified
java -jar htmlcompressor-1.5.3.jar --remove-intertag-spaces -o minified/ src/*.htm
cp -R src/config src/fonts src/pics src/sound src/styles src/util src/worker src/*.js minified
java -jar yuicompressor-2.4.8.jar --nomunge --preserve-semi --disable-optimizations -o '.css$:.css' minified/styles/*.css
java -jar yuicompressor-2.4.8.jar -o '.js$:.js' minified/util/*.js
java -jar yuicompressor-2.4.8.jar -o '.js$:.js' minified/worker/*.js
java -jar yuicompressor-2.4.8.jar -o '.js$:.js' minified/*.js
