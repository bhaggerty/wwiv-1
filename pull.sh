#!/bin/bash
#git remote add origin git@github.com:insidenothing/wwiv.git
#git init
cd /home/wwiv
svn checkout svn://svn.code.sf.net/p/wwiv/svn/trunk wwiv-svn
git add *
git commit -m "Nightly SVN Pull"
