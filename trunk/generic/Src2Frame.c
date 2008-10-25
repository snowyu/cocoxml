/*-------------------------------------------------------------------------
  Author (C) 2008, Charles Wang <charlesw123456@gmail.com>

  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License as published by the 
  Free Software Foundation; either version 2, or (at your option) any 
  later version.

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
  for more details.

  You should have received a copy of the GNU General Public License along 
  with this program; if not, write to the Free Software Foundation, Inc., 
  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  As an exception, it is allowed to write an extension of Coco/R that is
  used as a plugin in non-free software.

  If not otherwise stated, any source code generated by Coco/R (other than 
  Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/
#include  <stdio.h>
#include  "Frame.h"

static const char * license =
"/*-------------------------------------------------------------------------\n"
"  Author (C) 2008, Charles Wang <charlesw123456@gmail.com>\n"
"\n"
"  This program is free software; you can redistribute it and/or modify it\n"
"  under the terms of the GNU General Public License as published by the\n"
"  Free Software Foundation; either version 2, or (at your option) any\n"
"  later version.\n"
"\n"
"  This program is distributed in the hope that it will be useful, but\n"
"  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n"
"  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License\n"
"  for more details.\n"
"\n"
"  You should have received a copy of the GNU General Public License along\n"
"  with this program; if not, write to the Free Software Foundation, Inc.,\n"
"  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
"\n"
"  As an exception, it is allowed to write an extension of Coco/R that is\n"
"  used as a plugin in non-free software.\n"
"\n"
"  If not otherwise stated, any source code generated by Coco/R (other than\n"
"  Coco/R itself) does not fall under the GNU General Public License.\n"
"-------------------------------------------------------------------------*/\n";

int
main(int argc, const char * argv[])
{
    if (argc < 3) {
	fprintf(stderr, "Usage: %s OUTDIR INFILE ...\n", argv[0]);
	return 0;
    }
    return Frames('F', argv[1], NULL, license, NULL, NULL, argc - 2, argv + 2);
}
