/**
  Read STL files, and print their volume.
*/
#include <stdio.h>
#include "osl/stl.h"

int main(int argc,char *argv[]) {
	if (argc<=1) {
		printf("Usage: volume foo.stl [ bar.stl ... ]\n"
			"    Prints the volume in mm^3 and cm^3 of these STLs.\n");
		return 1;
	}

	double total=0.0;
	for (int argi=1;argi<argc;argi++) {
		const char *file=argv[argi];
		printf("%s: ",file); fflush(stdout);
		stl_loader stl(file);
		double volume=stl_volume(stl);

		for (size_t i=0;i<stl.size();i++)
			for (int vtx=0;vtx<3;vtx++)
				stl[i].vtx[vtx]+=vec3(10.0,-10.0,20.0); // shift
		
		double volume2=stl_volume(stl);
		if (fabs(volume-volume2)/volume>0.0001)
			printf("not very manifold (%.3f vs %.3f)\n",
				volume,volume2);
		else 
			printf("%d triangles, volume %.3f \n",
				(int)stl.size(),volume);
		total+=fabs(volume);
	}
	printf("Total volume: %.3f cubic <units>, %.3f cc (if units==mm)\n",
		total,total/1000.0);
	return 0;
}

