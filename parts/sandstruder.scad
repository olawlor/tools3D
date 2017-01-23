/**
"Sandstruder": extracts sand from hopper, meters it, 
and sends it down a sort of sand bowden tube.

Stepper motor: NEMA 17
	Held on the end of the sandstruder

Sand contact: auger drill bit
	1 inch diameter, 7.5 inch overall length.
	http://www.harborfreight.com/auger-bit-set-7-pc-68166.html
	Modified by drilling a 5mm hole in the end for the shaft of a NEMA 17 stepper motor. 
	(To drill the hardened end, I had to anneal the steel first, by heating to red hot with a propane torch and letting it cool slowly.)
	An M3x6mm setscrew retains the drill on the stepper motor shaft.  I added a blob of weld on the drill so the setscrew had more metal to contact.


Sand hopper: ammo can or polypropylene tub, with 1.25" hole cut in the bottom or lower side.

Dr. Orion Lawlor, lawlor@alaska.edu, 2017-01 (public domain)
*/

$fa=4; $fs=0.2; // smooth version

// Wall thickness
wall=3.0;

// Auger thru hole diameter
auger_dia=26;
auger_len=4.5*25.4;

// Auger sticks out this far
stickout=100;

// Shround around exruder inside hopper
shroud_len=25;

// Auger center
auger_start=[-stickout,0,0];
auger_dir=[0,90,0];

// Use a 3/8" ID bearing to support auger
//   (saves the stepper's bearings)
use_bearing=true;

// "Bowden tube": sand tube
bowden_dia=0.5*25.4; // hole for tube (1/2" HDPE)
bowden_start=[-25+wall,0,-30];
bowden_dir=auger_dir+[0,-45,0];
bowden_output=10; // length of final output area



// Representation for steel auger (for planning only)
module auger_itself() 
{
	// Auger render diameters
	auger_diaN=[8.45,8.45,9.42,25.4, 25.4,7.2,1.2];
	auger_lenN=[   0,  30,  59,  72, 180,180,193];

	color([0,1,1]) translate(auger_start) rotate(auger_dir)
	for (i=[0:len(auger_diaN)-1]) {
		translate([0,0,auger_lenN[i]])
			cylinder(d1=auger_diaN[i],d2=auger_diaN[i+1],h=auger_lenN[i+1]-auger_lenN[i]);
	}
}

/* XZ plane hopper side profile */
module hopper_profile_2D() {
	translate([200,0,0])
		square([400,400],center=true);
	
	/* // rounded 
	round=10;
	offset(r=+round) offset(r=-round)
	translate([0,-auger_dia/2-wall,0])
		square([200,200]);
	*/
}

/* Basic sandstruder geometry.
   Subtract the inside=1.0 version to get hollow surfaces right */
module sandstruder(inside=0.0) {
	outer_wall=(1-inside)*wall;

	/* Hopper interior. */
	translate([inside*wall,0.0,0])
	intersection() {
		wallsize=75;
		cube([wallsize+inside,wallsize+inside,wallsize+inside],center=true);
		rotate([90,0,0])
		linear_extrude(height=200+inside,convexity=2,center=true)
			hopper_profile_2D();
	}

	/* Auger wrapper */
	translate(auger_start) rotate(auger_dir)
		translate([0,0,-inside])
			cylinder(d=auger_dia+outer_wall*2,h=auger_len);

	/* Auger to bowden adapter */
	hull() {
		translate([bowden_start[0],auger_start[1],auger_start[2]]) rotate(auger_dir)
			translate([0,0,0+inside*wall])
			cylinder(d=auger_dia*1.3+outer_wall*2,h=25-inside*2*wall);
		translate(bowden_start) rotate(bowden_dir)
			translate([0,0,-inside])
				cylinder(d=bowden_dia+(1-inside)*2*wall,h=2*bowden_output);
	}
	
	/* Bowden output */
	translate(bowden_start) rotate(bowden_dir)
		translate([0,0,-bowden_output-inside])
			cylinder(d=bowden_dia+outer_wall*2,h=bowden_output+inside);
}

/* Overall sandstruder part */
module sandstruder_hollow() {
	difference() {
		sandstruder(0.0);
		sandstruder(1.0);
	}
			
	// Add shroud over entrance to auger
	// translate(auger_start) 
	rotate(auger_dir)
			difference() {
				cylinder(d=auger_dia+2*wall,h=shroud_len);
				translate([0,0,-1])
				cylinder(d=auger_dia,h=shroud_len+2);
			}
	
	// Add NEMA 17 stepper mount
	translate(auger_start) rotate(auger_dir)
	{
		nema17_mount(height=wall);
	}
	
	if (use_bearing) {
		// Add 3/8" ID bearing ring for auger
		bearing_dia=22.2;
		bearing_ht=7.1;
		bearing_recess=5; // ridge to support bearing
		bearing_thick=bearing_recess+bearing_ht;
		translate(auger_start) rotate(auger_dir)
		translate([0,0,45-bearing_thick])
		{
			difference() {
				// Body of bearing recess and ring
				cylinder(d=auger_dia+1,h=bearing_thick);
				
				// bearing recess in back
				translate([0,0,-0.05])
					cylinder(d=bearing_dia,h=bearing_ht);
				
				// thru hole
				translate([0,0,-1])
					cylinder(d=auger_dia*0.7,h=bearing_thick+2);
				
			}
		}
	}
}



module nema17_mount(height=4)
{
	linear_extrude(height=height,convexity=8)
	{
		difference() {
			motor_width = 42;
			// Main body
			offset(r=+5) offset(r=-5)
			translate([-motor_width/2,-motor_width/2])
				square([motor_width,motor_width]);

			// 4x M3 mounting bolt holes
			for (xsign=[-1,+1]) for (ysign=[-1,+1]) 
				translate([xsign*15.5, ysign*15.5, 0])
					circle(d=4.0);

			// Central clearance
			circle(r=11.5);
		}
	}
}

module sandstruder_section(angle) {
	intersection() {
		rotate([90+angle,0,0])
			sandstruder_hollow();
		translate([-200,-200,0]) cube([400,400,400]);
	}
	
}


// Show the auger (for planning)
// auger_itself(); 


// Print the struder in two halves, side by side.
sandstruder_section(0.0);
translate([0,-80,0])
	sandstruder_section(180.0);
