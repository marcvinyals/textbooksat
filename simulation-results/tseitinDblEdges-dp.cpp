#include<bits/stdc++.h>
using namespace std;

#include"tseitinDblEdges-common.cpp"

// this solution assumes that the top-left corner is a 1, all others vertices are 0.
void DECIDE(int l){
	cout << "d " << l << endl;
}

template<class T> void bruteforce(vector<int>L, T callback);
void MainLoop(int j,int msk);
void Solve(int j);

struct CBMainLoop {
	int j;
	CBMainLoop(int j):j(j){}
	void callback(int msk) {
		MainLoop(j,msk);
	}
};
struct CBNone { void callback(int msk) { } };

template<class T> void bruteforce(vector<int>L, T callback){
	int n = (int)L.size();
	for (int msk = 0; msk < (1<<n); msk++) {
		for (int bit = (msk==0)?(n-1):(__builtin_ctz(msk)-1); bit >= 0; bit--)
			DECIDE(-L[n-1-bit]);
		callback.callback(msk);
	}
}

void MainLoop(int j,int msk){
	DECIDE(Eid2[enc(w-1,j-2)][enc(w-1,j-1)]*((__builtin_popcount(msk)%2==1)?1:-1));
	if(j==2){
		vector<int>v;for(int i=0;i<w-1;i++)v.push_back(Eid1[enc(i,0)][enc(i+1,0)]);
		bruteforce(v,CBNone());
	}else{
		if(msk==0)Solve(j-1);
		else {
			vector<int>v;
			for(int i=0;i<w-1;i++)v.push_back(Eid1[enc(i,j-3)][enc(i,j-2)]),v.push_back(Eid2[enc(i,j-3)][enc(i,j-2)]);
			v.push_back(Eid1[enc(w-1,j-3)][enc(w-1,j-2)]);
			for(int i=0;i<w-1;i++)v.push_back(Eid1[enc(i,j-2)][enc(i+1,j-2)]);
			bruteforce(v,CBNone());
		}
	}
	vector<int>v;for(int i=0;i<w-1;i++)v.push_back(Eid1[enc(i,j-1)][enc(i+1,j-1)]);
	bruteforce(v,CBNone());
}

void Solve(int j){
	vector<int>v;
	for(int i=0;i<w-1;i++)v.push_back(Eid1[enc(i,j-2)][enc(i,j-1)]),v.push_back(Eid2[enc(i,j-2)][enc(i,j-1)]);
	v.push_back(Eid1[enc(w-1,j-2)][enc(w-1,j-1)]);
	bruteforce(v,CBMainLoop(j));
}

int main(){
	cin >> l >> w;
	assert(l > 1 && w > 1);
	genvarorder();
	Solve(l);
	cout << endl;
}
