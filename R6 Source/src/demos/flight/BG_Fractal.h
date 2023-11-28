#define maxIter 	9
#define Map(x, y)	map[(x) + mapS * (y)]
#define	random_prr(x,y) (rand() % ((y) - (x) + 1) + (x))
/*inline long random(long min, long max) 
{
	return(rand() % (max - min + 1) + min);
}*/

// prototypes
void	NormalizeMap(void);
inline	long MaxDeviation(long ic);
inline	void DeviatePoint(long o, long ic);
void	IterCalc(long s1, long s2, long a, long c);
void	InitMap(void);
void	InitMountains(void);
void	CalcMountains(void);
void	BG_CalculMountains(float *Altitude,int taille,long RandomKey);

