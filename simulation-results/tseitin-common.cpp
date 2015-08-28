map<int,map<int,int> > Eid;
int w,h;
int enc(int x,int y){return h*x+y;}
int nE=0;

// NOTE: do not change variable order
// (because forget-plugin hack in textbooksat depends on it)
void genvarorder(){
	for(int dir=0;dir<2;dir++)for(int x=0;x<w;x++)for(int y=0;y<h;y++){
		int x2=x+1-dir, y2=y+dir;
		if(0<=x2 && x2<w && 0<=y2 && y2<h){
			Eid[enc(x,y)][enc(x2,y2)]=++nE;
			Eid[enc(x2,y2)][enc(x,y)]=nE;
		}
	}
}
