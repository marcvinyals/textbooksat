#include<bits/stdc++.h>
using namespace std;

#include"tseitin-common.cpp"

// generates grid s.t. the top-left corner is a 1, all others vertices are 0.
int main(){
	cin >> w >> h;
	genvarorder();
	vector<vector<int>> S(w,vector<int>(h,0));
	S[0][0]=1;
	int CL=0;
	for(int x=0;x<w;x++)for(int y=0;y<h;y++){
		CL+=1<<((int)Eid[enc(x,y)].size()-1);
	}
	printf("p cnf %d %d\n",nE,CL);
	for(int x=0;x<w;x++)for(int y=0;y<h;y++){
		vector<int> v;
		for(auto p:Eid[enc(x,y)])v.push_back(p.second);
		for(int msk=0;msk<(1<<((int)v.size()));msk++){
			if(__builtin_popcount(msk)%2 != S[x][y]){
				for(int i=0;i<(int)v.size();i++){
					if(msk&(1<<i))cout<<-v[i]<<" ";else cout<<v[i]<<" ";
				}
				cout<<"0"<<endl;
			}
		}
	}
}
