# 7-Zip-Pro

is a free, open-source archiver for Windows similar to 7-Zip with:

 * new GUI written from scratch
 * new progress bar design
 * 7ze archive format
 * easy plugin interface for new codecs
 * a configurable list where codecs can be defined for file extensions

This project runs by raising funds for freelancers as I am not a programmer.
It can contain bugs which cannot be fixed by me.

# How can I contribute?

  You can have a look at the Issue list
  You can build the project or download the binaries (see below), run it on your system and report bugs or make enhancement proposals.

# Credits

Thanks for support, help and comments:

 * Igor Pavlov (author of original 7-Zip) 
 * PhonPhanom Sivilay (my freelancer) 
 * Manfred Slot (designer)
 * Bulat Ziganshin (thanks for the idea of grouping file extensions in a config file)
 * Ilya Grebnov (author of  LIBBSC 3.1.0)
 * Mario Scholz (author of  m7zRepacker)
 * Mathieu Chartier (author of MCM 0.83)
 * Matthias Stirner (author of packMP3)
 * Matthias Stirner (author of packPNM)
 * Moinak Ghosh (thanks for speeding up LZMA using prefetch instructions)
 * Siyuan Fu (author of CSC 3.2)
 * Francesco Nania 
 * Ilya Muravyov 

# License

Copyright 2014-2016 Stephan Busch

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

# Legal stuff

 * 7-Zip library
    Copyright (c) 1999-2014 Igor Pavlov
	LGPL license (except unRar source)
 * CSC 3.2 Final File Compressor, Ver.2011.03.01
    written by Siyuan Fu <fusiyuan2010@gmail.com>
	public domain
 * LIBBSC 3.1.0 
    Copyright (c) 2009-2012 Ilya Grebnov <Ilya.Grebnov@gmail.com>
	LGPL license
 * MCM 0.83
    Copyright (c) 2015 Mathieu Chartier
 	GPL license
 * packMP3 1.0e
    Copyright 2010-2014 Ratisbon University & Matthias Stirner
    	LGPL license
 * packPNM 1.6c
    Copyright 2006-2014 HTW Aalen University & Matthias Stirner
    	LGPL license

# Known Limitations

* there is no commandline executable and no 32-bit executable
* Compression settings are read from groups defined in the 7z.groups.ini file. 
  To change compression of filetypes defined in that file, you need to edit this .ini file.
* when using the 7z.groups.ini file, you will need approx. 4 GB of free memory
* in the sources, packJPG is mentioned but the codec integrated here is packMP3
* some plugin codecs (MCM, packPNM) are experimental and not used in 7z.groups.ini by default
* may contain bugs


# Donations

To donate, you can send money via PayPal to squeezechart@gmx.de
