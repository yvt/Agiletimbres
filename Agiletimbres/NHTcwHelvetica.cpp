#include "NHTcwHelvetica.h"
#include "NHTcwHelveticaData.h"

NHTcwHelvetica::NHTcwHelvetica():
NHTcwFont1Bit(NHTcwHelveticaData,
              NHTcwHelveticaDataIndexMap,
              NHTcwHelveticaDataWidthMap){
    topMargin=1;
    actualLineHeight=11;
}
