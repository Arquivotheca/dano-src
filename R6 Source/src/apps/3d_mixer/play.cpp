#include <MediaKit.h>
#include <string.h>
#include <stdio.h>
#include "play.h"
#include <math.h>

//-----------------------------------------------------------

short	glb_buffer[PLAY_SAMPLE_COUNT*2];
short	a_glb_buffer[PLAY_SAMPLE_COUNT*2];
long	data_sem;
long	buffer_sem;
long	a_data_sem;
long	a_buffer_sem;
char	about_producer;
char	ab_mode = 0;
//-----------------------------------------------------------

bool  record_func(void *data, char *buffer, size_t count, 
				  void *header);

void	init_a_s();
void	next_slice();
void	next_slice1();
//-----------------------------------------------------------


void init_stuff()
{
	BSubscriber *msub;
	BDACStream  *dac;

	data_sem = create_sem(0, "data_sem");
	buffer_sem = create_sem(1, "buffer_sem");
	dac = new BDACStream();
	
	dac->SetStreamBuffers(PLAY_BUFFER_SIZE, 4);
	dac->SetSamplingRate(44100);
	
	msub = new BSubscriber("subscriber");
	msub->Subscribe(dac);
	msub->EnterStream(NULL, false, 0, record_func, NULL, false);
}

//-----------------------------------------------------------


bool  record_func(void *data, char *buffer, size_t count, 
				  void *header)
{
	int16 *ptr = (int16 *)buffer;
	int16 datum;
	
	if (about_producer) {
		acquire_sem(a_data_sem);
		memcpy(ptr, a_glb_buffer, sizeof(a_glb_buffer));
		release_sem(a_buffer_sem);
	}
	else {
		acquire_sem(data_sem);
		memcpy(ptr, glb_buffer, sizeof(glb_buffer));
		release_sem(buffer_sem);
	}
	
	return true;
	
}

//-----------------------------------------------------------

long	about_p(void *p)
{
	long	i;
	float	t0;
	float	t1;
	float	dt;

	t0 = system_time();
	while(1) {
		if (ab_mode == 3)
			goto out;
		while(ab_mode == 0) {
			snooze(10000);
		}

		acquire_sem(a_buffer_sem);
		if (ab_mode == 2)
			next_slice1();
		if (ab_mode == 1)
			next_slice();
		release_sem(a_data_sem);
		t1 = system_time();

		dt = (t1-t0);
		dt /= 1000000.0;
	}

out:;
	about_producer = 0;
}

//-----------------------------------------------------------


void	do_about_sound()
{
	a_data_sem = create_sem(0, "data_sem");
	a_buffer_sem = create_sem(1, "buffer_sem");
	
	init_a_s();
	about_producer = 1;
	resume_thread(spawn_thread(about_p,"about_p",B_REAL_TIME_PRIORITY,0));
}

//-----------------------------------------------------------

#define	N	27

//-----------------------------------------------------------

double			freq[N];
double			p[N];
double			speed[N];
float			boo[100];
float			targ[N];
float			boo1[100];
short			vsin[0x400];
extern short	wave[1024];
long			gv;
float			gain;

//-------------------------------------------------------------------

void	init_a_s()
{
	long	i;
	float	k1,k2,k3;
	short	v;
	short	v1;

	gain = 0.8;

	for (i = 0; i < 0x400; i++) {
		//vsin[i] = 9 * (-128 + (i) % 256);

		v = wave[i];
		vsin[i] = 0.5*v;
	}

	for (i = 0; i < N*0.5; i++) {
		freq[i] = 900 + rand() % 1500;
		freq[i] /= 5000.0;
		speed[i] = 98000 + (rand() % 12000);
		p[i] = 0;
	}


	for (i = 0; i < N*0.15; i++) {
		freq[i] = 450 + rand() % 300;
		freq[i] /= 5000.0;
		speed[i] = 98000 + (rand() % 12000);
		p[i] = 0;
	}

	for (i = N*0.5; i < N; i++) {
		freq[i] = 8500 + rand() % 100;
		freq[i] /= 5000.0;
		speed[i] = 95000 + (rand() % 12000);
		p[i] = 0;
	}
	for (i = N*0.8; i < N; i++) {
		freq[i] = 17500 + rand() % 100;
		freq[i] /= 5000.0;
		speed[i] = 95000 + (rand() % 12000);
		p[i] = 0;
	}


	gv = 0;


	k1 = 8;	
	k2 = 2.0;
	k3 = 1.0;

	for (i = 0; i < N; i++)
		targ[i] = k1;

	for (i = 0; i < N/3; i+=3) {
		targ[i] = k1 + ((float)(rand() & 0x7fff) /950000.0);
		targ[i+1] = k1 *(8.0/12.0) + ((rand() & 0x7fff) /950000.0);
		targ[i+2] = k1 *(5.0/12.0) + ((rand() & 0x7fff) /950000.0);
	}

	for (i = N/3; i < N*0.666; i+=3) {
		targ[i] = k2 + ((rand() & 0x7fff) /950000.0);
		targ[i+1] = k2 * (5.0/12.0) + ((rand() & 0x7fff) /950000.0);
		targ[i+2] = k2 * (8.0/12.0) + ((rand() & 0x7fff) /950000.0);
	}

	for (i = N*0.666; i < N; i+=3) {
		targ[i] = k3 + ((rand() & 0x7fff) /950000.0);
		targ[i+1] = k3 * (5.0/12.0) + ((rand() & 0x7fff) /950000.0);
		targ[i+2] = k3 * (8.0/12.0) + ((rand() & 0x7fff) /950000.0);
	}

}

//-------------------------------------------------------------------

void	set_channel_f(long c, float f)
{
	freq[c] = fabs(f);
}

//-------------------------------------------------------------------

void	next_slice1()
{
	long	v;
	short	vv;
	long	v1;
	long	i;
	long	acc;
	long	uv;
	
	gain = 1.0;
	for (v = 0; v < PLAY_SAMPLE_COUNT; v++) {
		acc = 0;
		for (i = 0; i < N/2; i++) {
			v1 = p[i] += freq[i];
			acc += vsin[v1 & 0x3ff];
		}

		acc *= (2.0/N);
		vv = acc;
		a_glb_buffer[v*2] = gain*vv;

		acc = 0;

		for (i = N/2; i < N; i++) {
			v1 = p[i] += freq[i];
			acc += vsin[v1 & 0x3ff];
		}
		acc *= (2.0/N);

		vv = acc;

		a_glb_buffer[v*2+1] = gain*vv;
	}
}


//-------------------------------------------------------------------

void	next_slice()
{
	long	v;
	short	vv;
	long	v1;
	long	i;
	long	acc;
	long	uv;
	

	for (v = 0; v < PLAY_SAMPLE_COUNT; v++) {
		acc = 0;
		for (i = 0; i < N/2; i++) {
			v1 = p[i] += freq[i];
			acc += vsin[v1 & 0x3ff];
		}

		acc *= (2.0/N);
		vv = acc;
		a_glb_buffer[v*2] = gain*vv;

		acc = 0;

		for (i = N/2; i < N; i++) {
			v1 = p[i] += freq[i];
			acc += vsin[v1 & 0x3ff];
		}
		acc *= (2.0/N);

		vv = acc;


		a_glb_buffer[v*2+1] = gain*vv;

		if ((v&0x1f) == 0) {
			gv += 0x1f;
			for (i = 0; i < N; i++) {
				freq[i] = ((freq[i]*speed[i]) + targ[i])/(speed[i]+1.0);
				speed[i] *= 0.99915;
			}
			gain += 0.001;
			if (gv > 320000) {
				gain -= 0.0045;
				if (gain < 0.1) {
					gain = 0;
					ab_mode = 0;
				}
			}
		}
	}
}

//-------------------------------------------------------------------

short	wave[1024] = {
41,650,1258,1566,1874,2528,3181,3651,4121,4135,4150,3852,3554,3690,3827,4386,4946,
5533,6121,6411,6701,6357,6013,5821,5628,6007,6387,6774,7162,7059,6957,6801,6646,
6162,5678,4849,4019,3837,3655,4008,4361,3861,3362,2442,1521,1240,958,1967,2976,
3793,4610,4446,4282,4386,4491,4861,5231,5453,5676,5894,6112,6358,6605,6503,6402,
6251,6101,5739,5378,5777,6176,6577,6978,7523,8069,8293,8517,7205,5893,5192,4490,
4672,4855,4646,4437,3806,3174,3196,3218,3452,3686,4470,5255,5498,5742,5000,4260,
3751,3243,3123,3004,2948,2893,2911,2929,3440,3952,3807,3664,2387,1111,500,-111,
889,1889,1971,2052,2313,2574,2831,3088,2435,1782,1497,1213,1363,1513,1287,1061,
346,-368,59,487,623,759,1127,1496,1822,2148,1903,1658,1153,648,84,-480,
-368,-256,-17,221,465,709,376,43,-470,-983,-1076,-1169,-788,-407,-496,-586,
-915,-1244,-1750,-2256,-3197,-4138,-4728,-5317,-5253,-5189,-5466,-5743,-6661,-7579,-8304,-9029,
-8656,-8284,-7198,-6112,-6090,-6069,-5952,-5836,-5262,-4689,-4200,-3712,-3203,-2695,-2054,-1414,
-1084,-754,-734,-715,-880,-1046,-674,-301,528,1357,1870,2384,2995,3606,2702,1799,
909,20,197,373,224,76,-435,-947,-1071,-1196,-973,-751,-183,384,809,1235,
660,85,-588,-1261,-1631,-2002,-2076,-2151,-2174,-2197,-1641,-1085,-666,-247,-1105,-1964,
-2972,-3981,-3012,-2043,-1406,-769,-691,-613,-182,248,-328,-904,-1121,-1337,-1401,-1466,
-1565,-1665,-2361,-3057,-2899,-2742,-2206,-1670,-1137,-605,173,953,1126,1299,1316,1334,
987,640,1073,1505,2035,2564,3081,3597,3711,3826,3543,3261,3249,3238,3699,4160,
4428,4696,4574,4451,4224,3997,3293,2588,2200,1811,1863,1914,2022,2129,1582,1034,
333,-369,-340,-312,760,1832,2070,2308,2146,1983,2335,2688,2996,3304,3723,4143,
4394,4644,4857,5070,4821,4572,4185,3798,3774,3750,4489,5228,5650,6071,6668,7266,
6787,6308,4991,3674,3750,3827,3719,3612,3167,2722,2427,2132,2238,2344,2796,3248,
3741,4234,3960,3687,3158,2630,2295,1960,1929,1899,1916,1933,2003,2073,2588,3103,
2710,2317,902,-513,-475,-438,327,1092,1087,1082,1619,2156,1745,1334,954,574,
786,998,1184,1370,930,490,43,-404,-69,265,569,873,1463,2052,1963,1873,
1804,1734,1119,505,193,-118,173,465,600,736,854,973,774,574,248,-78,
-97,-116,311,739,701,662,459,257,-549,-1356,-2113,-2870,-3041,-3212,-3224,-3236,
-3692,-4148,-4850,-5553,-6254,-6955,-6508,-6061,-5378,-4695,-4914,-5134,-4683,-4232,-3927,-3622,
-3110,-2599,-1925,-1250,-575,100,265,430,271,112,49,-14,679,1373,2083,2792,
3231,3669,4044,4420,3071,1721,1165,608,804,999,674,348,-150,-648,-869,-1091,
-1102,-1113,-733,-354,-229,-104,-755,-1407,-1917,-2427,-2801,-3175,-3152,-3129,-3209,-3289,
-2901,-2513,-2135,-1757,-2579,-3402,-4121,-4841,-4125,-3410,-2949,-2489,-1993,-1497,-1330,-1162,
-1841,-2519,-2631,-2743,-2511,-2279,-2385,-2491,-3276,-4062,-4116,-4171,-3868,-3567,-2593,-1620,
-1114,-609,-456,-304,-145,13,-454,-922,-567,-213,308,829,1052,1275,1302,1329,
1304,1281,1120,960,1416,1872,2119,2366,2309,2251,1991,1731,874,18,-325,-668,
-474,-279,-472,-665,-940,-1215,-1822,-2430,-2787,-3145,-2142,-1140,-1025,-911,-773,-635,
-418,-200,-123,-46,421,888,1413,1938,2056,2174,1774,1374,942,510,579,647,
1494,2340,2536,2733,3387,4041,3494,2948,1767,586,630,675,667,658,229,-200,
-503,-805,-745,-685,-387,-88,488,1065,811,557,269,-19,-376,-733,-765,-798,
-767,-736,-573,-409,56,523,149,-223,-1220,-2216,-2296,-2377,-1756,-1137,-786,-435,
0,434,-71,-576,-844,-1113,-931,-749,-507,-265,-672,-1078,-1524,-1971,-1937,-1903,
-1375,-848,-81,686,589,492,427,363,-409,-1182,-1546,-1911,-1735,-1559,-1527,-1495,
-1606,-1716,-1930,-2144,-2422,-2701,-2579,-2457,-2129,-1802,-2043,-2285,-2510,-2735,-3593,-4451,
-5192,-5933,-6046,-6159,-6460,-6761,-7267,-7774,-8355,-8937,-9694,-10452,-10026,-9600,-9163,-8727,
-8620,-8514,-8131,-7749,-7604,-7459,-6776,-6093,-5270,-4447,-3711,-2976,-2843,-2710,-2974,-3238,
-3245,-3252,-2240,-1228,-605,16,602,1189,1136,1084,-109,-1303,-1332,-1361,-1250,-1138,
-1408,-1677,-1970,-2264,-2286,-2307,-2167,-2026,-1607,-1187,-1419,-1651,-2047,-2442,-2824,-3205,
-3326,-3447,-3380,-3314,-3382,-3450,-2787,-2124,-2129,-2134,-2846,-3558,-3754,-3950,-3246,-2543,
-2224,-1906,-1457,-1009,-1334,-1659,-2148,-2638,-2375,-2113,-1926,-1738,-1767,-1796,-2435,-3074,
-3110,-3147,-2746,-2346,-1405,-464,78,621,1002,1383,1114,844,595,346,894,1442,
1738,2034,2426,2819,3030,3241,3202,3164,3208,3252,3809,4367,4298,4229,4195,4162,
3763,3364,2825,2286,2342,2399,2218,2038,1804,1570,1165,760,166,-428,-324,-220,
325,871,838,805,1149,1493,1505,1516,1692,1867,2456,3045,3477,3908,4143,4378,
3793,3208,2569,1930,2406,2882,3430,3978,4431,4885,5254,5623,4357,3092,2540,1988,
2304,2620,2383,2146,1835,1523,1354,1186,1047,909,1173,1436,1378,1319,1019,720,
504,287,-3,-293,-73,147,-212,-571,-365,-160,128,417,-218,-854,-1652,-2450,
-2242,-2035,-1744,-1453,-1251,-1049,-914,-780,-1482,-2184,-2149,-2115,-1833,-1551,-1212,-872,
-1343,-1814,-2452,-3091,-3148,-3206,-2615,-2024,-1594,-1164,-1024,-884,-996,-1108,-1979,-2851,
-2936,-3022,-2875,-2728,-2576,-2424,-2323,-2223,-2258,-2294,-2695,-3096,-2728,-2361,-2223,-2084,
-2222,-2361,-2450,-2540,-3442,-4345,-4463,-4581,-4927,-5273,-5771,-6268,-6620,-6973,-7706,-8440,
-9115,-9791,-9387,-8984,-8722,-8461,-8283,-8105,-7595,-7086,-6965,-6844,-6023,-5203,-4293,-3382,
-2507,-1632,-1349,-1066,-1621,-2175,-1796,-1416,-420,575,1104,1633,2535,3438,3158,2878,
1802,727,833,939,1096,1253,1085,916,839,763,521,279,345,411,632,853
};