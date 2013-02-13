#include <typeinfo>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include "FF.h"
#include "LM.h"
#include "RO.h"
#include "PT.h"
#include "WP.h"

using namespace std;

string iniPath;
vector<FF*> ffVec;

void Output()
{
  ofstream strme(iniPath.c_str());
  stringstream weightStrme;

  weightStrme << "\n\n[weight]" << endl;

  strme << "[input-factors]" << endl;
  strme << "0" << endl;

  strme << "[mapping]" << endl;
  strme << "0 T 0" << endl;

  strme << "[distortion-limit]" << endl;
  strme << "6" << endl;

  strme << "\n\n[feature]" << endl;
  for (size_t i = 0; i < ffVec.size(); ++i) {
    const FF &ff = *ffVec[i];

    ff.Output(strme);
    ff.OutputWeights(weightStrme);
  }

  strme << weightStrme.str();

  strme.close();
}

int main(int argc, char **argv)
{
	WP *model = new WP("");
	ffVec.push_back(model);	

  for (int i = 0; i < argc; ++i) {
    string key(argv[i]);
    
    if (key == "-phrase-translation-table") {
      ++i;
      PT *model = new PT(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-reordering-table") {
      ++i;
      RO *model = new RO(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-lm") {
      ++i;
      LM *model = new LM(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-config") {
      ++i;
      iniPath = argv[i];
    }
  }

  Output();
}




