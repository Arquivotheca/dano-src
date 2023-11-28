#include "KeymapAddOn.h"

void KeyView::GenerateLayout(int32) {
	for (int j=0;j<7;j++) {
		for (int i=0;i<23;i++) {
			keyframe[i][j]=BRect(i*(Bounds().Width()+1)/23,j*(Bounds().Height()+1)/7,(i+1)*(Bounds().Width()+1)/23-1,(j+1)*(Bounds().Height()+1)/7-1);
		}
		float hsize=1;
		float hoffset=0;
		switch(j) {
			case 0 : {
				for (int i=0;i<23;i++) {
					switch (i) {
						case 1 : {
							hoffset=1;
							break;
						}
						case 5 : {
							hoffset=1.5;
							break;
						}
						case 9 : {
							hoffset=2;
							break;
						}
						case 13 : {
							hoffset=2.5;
							break;
						}
					}
					keyframe[i][j]=BRect((i+hoffset)*(Bounds().Width()+1)/23,j*(Bounds().Height()+1)/7,(i+hoffset+hsize)*(Bounds().Width()+1)/23-1,(j+1)*(Bounds().Height()+1)/7-1);
				}
				linelength[j]=16;
				break;
			}
			case 1 : {
				linelength[j]=0;
				break;
			}
			case 2 : {
				float hsize=1;
				float hoffset=0;
				for (int i=0;i<23;i++) {
					switch (i) {
						case 15 : {
							hoffset=.5;
							break;
						}
						case 18 : {
							hoffset=1;
							break;
						}
					}
					keyframe[i][j]=BRect((i+hoffset)*(Bounds().Width()+1)/23,j*(Bounds().Height()+1)/7,(i+hoffset+hsize)*(Bounds().Width()+1)/23-1,(j+1)*(Bounds().Height()+1)/7-1);
				}
				linelength[j]=22;
				break;
			}
			case 3 : {
				for (int i=0;i<23;i++) {
					switch (i) {
						case 1 : {
							hoffset=.5;
							break;
						}
						case 13 : {
							hoffset=2.5;
							break;
						}
						case 16 : {
							hoffset=3;
							break;
						}
					}
					if (i==0) {
						hsize=1.5;
					} else {
						hsize=1;
					}
					keyframe[i][j]=BRect((i+hoffset)*(Bounds().Width()+1)/23,j*(Bounds().Height()+1)/7,(i+hoffset+hsize)*(Bounds().Width()+1)/23-1,(j+1)*(Bounds().Height()+1)/7-1);
				}
				linelength[j]=19;
				break;
			}
			case 4 : {
				for (int i=0;i<23;i++) {
					switch (i) {
						case 1 : {
							hoffset=.75;
							break;
						}
						case 12 : {
							hoffset=7;
							break;
						}
					}
					if (i==0) {
						hsize=1.75;
					} else {
						hsize=1;
					}
					keyframe[i][j]=BRect((i+hoffset)*(Bounds().Width()+1)/23,j*(Bounds().Height()+1)/7,(i+hoffset+hsize)*(Bounds().Width()+1)/23-1,(j+1)*(Bounds().Height()+1)/7-1);
				}
				linelength[j]=15;
				break;
			}
			case 5 : {
				for (int i=0;i<23;i++) {
					switch (i) {
						case 1 : {
							hoffset=1.25;
							break;
						}
						case 12 : {
							hoffset=4.5;
							break;
						}
						case 13 : {
							hoffset=6;
							break;
						}
					}
					if (i==0) {
						hsize=2.25;
					} else if (i==11) {
						hsize=2.75;
					} else {
						hsize=1;
					}
					keyframe[i][j]=BRect((i+hoffset)*(Bounds().Width()+1)/23,j*(Bounds().Height()+1)/7,(i+hoffset+hsize)*(Bounds().Width()+1)/23-1,(j+1)*(Bounds().Height()+1)/7-1);
				}
				linelength[j]=16;
				break;
			}
			case 6 : {
				for (int i=0;i<23;i++) {
					hoffset+=hsize-1;
					switch (i) {
						case 8 : {
							hoffset+=.5;
							break;
						}
						case 11 : {
							hoffset+=.5;
							break;
						}
					}
					if (i<8) {
						if (i==3) {
							hsize=6.25;
						} else {
							hsize=1.25;
						}
					} else {
						if (i==11) {
							hsize=2;
						} else {
							hsize=1;
						}
					}
					keyframe[i][j]=BRect((i+hoffset)*(Bounds().Width()+1)/23,j*(Bounds().Height()+1)/7,(i+hoffset+hsize)*(Bounds().Width()+1)/23-1,(j+1)*(Bounds().Height()+1)/7-1);
				}
				linelength[j]=13;
				break;
			}
		}
	}
}
