#include <support2/Value.h>
#include <support2/Team.h>

#include <stdio.h>

using namespace B::Support2;

static atom_ref<BImage> g_addonImage;

extern "C" IBinder::ptr _start_root_(const BValue & params, image_id image);

extern "C" IBinder::ptr
_start_root_(const BValue & params, image_id image)
{
	g_addonImage = new BImage(image);
	printf("new addon loaded.  I am image %d\n", (int) image);
	return root(params);
}

BImageRef::BImageRef()
	:m_image(g_addonImage.promote())
{
	printf("ImageRef acquired\n");
}

BImageRef::~BImageRef()
{
	printf("ImageRef released\n");
}
