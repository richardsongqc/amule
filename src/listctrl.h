#include <wx/version.h>

#if wxCHECK_VERSION(2, 6, 0)
	#include "extern/listctrl.cvs.h"
#else
	#error Unsupported version of wxWidgets used!
#endif
