#!/bin/sh
arep=AGReader
cd ..
if [ -f ${arep}.tgz ] ; then
	rm -i ${arep}.tgz
fi
echo -e -n "\033[1marchiving $arep\033[0m..."
tar zcf ${arep}.tgz $arep
echo "done."

