/**
  Trivial STL file (3D printer stereolithography) utilities.
  Self-contained, except for 3D float vector class vec3.
  
  Dr. Orion Lawlor, lawlor@alaska.edu, 2014-11-07 (Public Domain)
*/
#ifndef __OSL_STL_H
#define __OSL_STL_H

#include <stdint.h> /* for uint32_t and such */
#include <cstring> /* for strncmp */
#include <vector> /* list of triangles */
#include <map> /* STL */
#include <fstream> /* for I/O */
#include <iostream> /* debugging */
#include "vec4.h" /* osl/vec4 declares "vec3", a 3D float vector */


/** Binary format triangle: ASSUMES little-endian machine. */
struct stl_triangle {
	vec3 normal; // surface normal, or (0,0,0) for right hand rule
	vec3 vtx[3]; // vertex locations

	stl_triangle() :normal(0.0) {}
};

/** Binary format header: ASSUMES little-endian machine. */
struct stl_file_header {
	char comment[80]; // If this starts with "solid", it's ASCII format
	uint32_t ntri; // triangle count: little endian integer
};

/**
  Loads an STL file into a list of triangles.
  Automatically works with either binary or ascii files.
*/
class stl_loader : public std::vector<stl_triangle> {
public:
	std::string comment; // comment field
	
	stl_loader(const char *fileName) {
		read(fileName);
	}
	
	// Add this STL file's contents to us.
	void read(const char *fileName)
	{
		std::ifstream bfile(fileName,std::ios_base::binary);
		stl_file_header header;
		bfile.read((char *)&header,sizeof(header));
		if (!bfile) return; // fail
		
		// Some stupid files start with "solid", but are really binary data.
		//  Heuristic: if it's got binary data, it's binary.
		bool has_binary=false;
		for (int i=0;i<1000;i++) {
			unsigned char c='a';
			if (bfile.read((char *)&c,1)) {
				if (c<' ' && c!='\n' && c!='\r' && c!='\t') has_binary=true;
				else if (c>'~') has_binary=true; // <- or Unicode?
			}
		}
		
		if ((!has_binary) && std::strncmp(header.comment,"solid",5)==0) 
		{ /* ASCII format */
			read_ascii(fileName);
		} else { 
			read_binary(fileName);
		}
	}
	
	void read_binary(const char *fileName) {
		std::ifstream bfile(fileName,std::ios_base::binary);
		stl_file_header header;
		bfile.read((char *)&header,sizeof(header));
		comment=std::string(header.comment,sizeof(header.comment));
		/* binary format: read all the triangles */
		size_t ntri=header.ntri;
		for (size_t i=0;i<ntri;i++) {
			// Read i'th triangle
			stl_triangle t;
			bfile.read((char *)&t,sizeof(t));
			push_triangle(t);
			
			// Read 2 bytes of attribute data
			//  This is *not* a byte count, evidently!
			uint16_t attrBytes=0;
			bfile.read((char *)&attrBytes,sizeof(attrBytes));
		}
	}
	
	// Read all the triangles from this ASCII STL file
	void read_ascii(const char *fileName) {
		std::ifstream afile(fileName); // reopen in text mode
		std::string token;
		afile>>token;
		if (token!="solid") return; // not an ASCII file
		std::getline(afile,comment); // skip rest of first line
		stl_triangle t;
		int cv=0;
		while (true) { // facet loop
			afile>>token; // read next token
			if (!afile) break; // unexpected end of file!
			else if (token=="facet") continue;
			else if (token=="normal") t.normal=read_xyz(afile);
			else if (token=="outer" || token=="loop") { cv=0; continue; }
			else if (token=="vertex") {
				if (cv<3)
					t.vtx[cv++]=read_xyz(afile);
				else
					std::cerr<<fileName<<": WARNING non-triangle vertices in facet "<<this->size()<<"\n";
			}
			else if (token=="endloop") continue; 
			else if (token=="endfacet") push_triangle(t);
			else if (token=="endsolid") break; // end of file token
			else std::cerr<<fileName<<": WARNING unknown '"<<token<<"'\n";
		}
	}
	
	// Read one X Y Z position from this ASCII file
	vec3 read_xyz(std::ifstream &afile) {
		float x,y,z;
		afile>>x>>y>>z;
		return vec3(x,y,z);
	}
	
	// Sanity-check and add this triangle
	void push_triangle(const stl_triangle &t) {
		float normalLength=length(t.normal);
		if (!(normalLength<1.0e5)) std::cerr<<"WARNING bad normal length "<<normalLength<<" on triangle "<<this->size()<<"\n";
		
		if (sane(t.vtx[0]) && sane(t.vtx[1]) && sane(t.vtx[2]))
			this->push_back(t);
	}
	
	// Sanity-check this XYZ position
	bool sane(const vec3 &v) const {
		float mag=length(v);
		if (mag>1.0e7) {
			std::cerr<<"WARNING: skipping huge vertex "<<v.x<<","<<v.y<<","<<v.z<<"\n";
			return false;
		}
		if (mag!=mag) {
			std::cerr<<"WARNING: skipping NaN vertex "<<v.x<<","<<v.y<<","<<v.z<<"\n";
			return false;
		}
		return true;
	}
};


// Calculate volume enclosed by this list of STL triangles.
//  ASSUMES shape is closed, AND triangle orientations are consistent.
double stl_volume(const std::vector<stl_triangle> &tri) 
{
	double sum=0.0;
	for (size_t i=0;i<tri.size();i++) {
		const stl_triangle &t=tri[i];
		// This is the volume of tet from origin to triangle:
		sum+=dot(t.vtx[0],cross(t.vtx[1],t.vtx[2]))*(1.0/6.0);
		//  See http://en.wikipedia.org/wiki/Tetrahedron#Volume
		
		/* Center of mass fail: 
		const vec3 &A=t.vtx[0], &B=t.vtx[1], &C=t.vtx[2];
		vec3 RA=A-C, RB=B-C;
		float thingy=0.5*dot(C,C)+2.0/3*dot(C,RA)+1.0/3*dot(C,RB)+
			1.0/4*dot(RA,RA) + 1.0/4*dot(RA,RB) +
			1.0/12*dot(RB,RB);
		sum+=(thingy*RA+thingy*RB).x*normalize(cross(RA,RB)).x;
		*/
		
		//vec3 n=normalize(cross(B-A,C-A)); // surface normal
		//sum+=n.x*length(cross(B-A,C-A))/2.0;  // area
		// sum+=cross(B-A,C-A).x/2.0 * (A+B+C).x^2/3.0;
		
		// This is from the Divergence Theorem:
		//    https://www.cs.uaf.edu/2015/spring/cs482/lecture/02_20_boundary.html
		// const vec3 &A=t.vtx[0], &B=t.vtx[1], &C=t.vtx[2];
		// sum+=(1.0/6.0)* dot(vec3((A+B+C).x,0,0), cross(B-A,C-A));
	}
	return sum;
}



#endif

