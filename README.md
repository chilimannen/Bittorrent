BitTorrent Client
===============

BitTorrent Client, Written in C for Linux.

Development Started: 2014-04-16
Development Stopped: 2014-05-29

The MIT License (MIT)

Copyright (c) 2014 Ahmed Al-Battat, Muhamed Aljic, 
		   Robin Duda, Anton Liljeberg,
		   Gustaf Nilstadius.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Developed For KTH-STH Haninge.

   [Don't use your favorite text editor. Use 'more', it's super effective!]
================================================================================

Cookie-Crumb Bittorrent - v1.0b (First beta version)

    == Table of Contents ==
      1. Known Issues.
      2. System requirements.
      3. Installing the Client..
      4. Modifying the source code.
    == == == == == == == == == ==

	1. Known Issues

		- Some clients seem to be rejecting our client. 
		  [KTorrent, Transmission]
		  Possible reasons for this may be the lacking support 
		  of µTP on our end.
		  (Tested and developed against µTorrent and BitTorrent.)

		- Does not compile with GTK3, use GTK2 when compiling!

		- GTK Produces some warnings at runtime, needs to be addressed.

		- The GTK file dialog will not find any files, relaunching the
		  client will fix this.

		- GTK sometimes fails to scale the columns to their correct 
		  size.

	2. System requirements

		Hardware
		  - An ethernet card and some other hardware which you already
		    should have installed in your system if you are reading
		    this.
		
		Dependencies
		  - Library pthread is used for threading.
		  - GTK2 is required for the graphical user interface.
		  - For the hashing algorithm SHA1, the library lcrypto is
		    required.

		Note on resource usage
		  - Should you find the software to be hogging your system, look
		    into the gui_update_thread() and more specifically the 
		    list_update() function. Updating the values in the graphical
		    user interface requires an expensive GTK API call. Reducing 
		    the FPS-define in GUI.c is another alternative.

	3. Installing the client

		3.1 Make sure you have the latest version from our Git repo.
			https://github.com/kthchatt/BitTorrent.git
		3.2 Read the README file, you should be reading it right now.
		3.3 Set your working directory to ccBitTorrent folder.
		3.4 Run the make - file.
		3.5 Download every torrent. Much download, many peer. very seed.

	4. Modifying the source code.

		* Are you tired of your bittorrent client?
		* Are you in desperate need of an upgrade.. ?
		* Did we not make the source code pretty enough!? 

		Well, you are in luck my friend. This project is OPEN SOURCE.

		4.1 Quick Overview

			GUI.c
			  Modding the user - interface.
		
			bencodning.c
			   Metainfo & Bencoding.
			
			createfile.c
			   File creation: current mode is allocate 
			   pre-download, you might want on-demand allocations.
			
			peerwire.c
			   Implementing the peerwire protocol, this is
			   where you look if you want to implement PEX,
			   streaming or µTP and anything else you can imagine.
			
			tracker.c
			   Implement DHT here.

			swarm.c
			   Implement ZeroConf here.
			   
			scrape.c/announce.c
			   Implement UDP trackers here.
			

Author: Robin Duda, 2014-05-28

------------------------------ END OF MAN FILE ---------------------------------



