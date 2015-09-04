#include<bits/stdc++.h>
using namespace std;

#include"tseitin-common.cpp"

// this solution assumes that the top-left corner is a 1, all others vertices are 0.
void DECIDE(int l){
	cout << "d " << l << " ";
}

void Solve(int w){
	if(w==2){
		for(int msk=0;msk<(1<<(h-1));msk++)
			for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
				DECIDE(-Eid[enc(0,bit)][enc(1,bit)]);
	}else{
		for(int msk=0;msk<(1<<h);msk++){
			for(int bit=(msk==0)?(h-1):(__builtin_ctz(msk)-1);bit>=0;bit--)
				DECIDE(-Eid[enc(w-3,bit)][enc(w-2,bit)] * ((msk%2==0 && __builtin_popcount(msk)%2==1 && bit==0)?-1:1));
			if(msk==0) Solve(w-2); else
			if(msk%2==0){
				if(w>4){
					for(int msk=0;msk<(1<<(h-1));msk++){
						for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
							DECIDE(-Eid[enc(w-4,bit)][enc(w-3,bit)]);
						for(int msk2=0;msk2<(1<<(h-1));msk2++)
							for(int bit=(msk2==0)?(h-2):(__builtin_ctz(msk2)-1);bit>=0;bit--)
								DECIDE(-Eid[enc(w-5,bit)][enc(w-4,bit)]);
					}
				} else
					for(int msk=0;msk<(1<<(h-1));msk++)
						for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
							DECIDE(-Eid[enc(0,bit)][enc(1,bit)]);
			}else
				for(int msk=0;msk<(1<<(h-1));msk++)
					for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
						DECIDE(-Eid[enc(w-2,bit)][enc(w-1,bit)]);
		}
	}
}

int main(){
	cin >> w >> h;
	assert(w > 1 && h > 1);
	assert(w % 2 == 0);
	genvarorder();
	Solve(w);
	cout << endl;
}
