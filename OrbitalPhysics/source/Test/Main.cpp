#include "Util/Util.h"

namespace Limnova
{

namespace Test
{

	void Main()
	{
		LV_LOG(Fmt::Format("Hello from OrbitalPhysicsTest!"));
	}

}

}

int main(int argc, int** argv)
{
	Limnova::Test::Main();

	return 0;
}
