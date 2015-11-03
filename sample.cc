#include "sample.h"


// Constructors

sample::sample(std::string stName, std::string niceName)
  : name_storage(stName),
	name_table(niceName),
	name_legend(niceName),
	is_Data(false),
	hist_color(kBlack)
{
}

sample::sample(std::string stName, std::string niceName, short int color, bool isdata=false)
  : name_storage(stName),
	name_table(niceName),
	name_legend(niceName),
	is_Data(isdata),
	hist_color(color)
{
}

sample::sample(std::string stName, std::string tabName, std::string legName)
  : name_storage(stName),
	name_table(tabName),
	name_legend(legName),
	is_Data(false),
	hist_color(kBlack)
{
}

sample::sample(std::string stName, std::string tabName, std::string legName, short int color, bool isdata=false)
  : name_storage(stName),
	name_table(tabName),
	name_legend(legName),
	is_Data(isdata),
	hist_color(color)
{
}

// Other member functions

TString sample::GetIntName()   { return static_cast<TString>(name_storage); }
TString sample::GetTableName() { return static_cast<TString>(name_table);   }
TString sample::GetLegName()   { return static_cast<TString>(name_legend);  }
bool    sample::IsData()       { return is_Data;      }
short int sample::GetColor()   { return hist_color;   }

void sample::SetNiceName(std::string name) { name_table=name; name_legend=name; }
void sample::SetColor(short int color)     { hist_color=color; }
void sample::SetDataFlag(bool isdata = true)      { is_Data=isdata; }
