
#include <support2/TLS.h>
#include <support2/Looper.h>
#include <support2/Binder.h>
#include <support2/IValueStream.h>
#include <support2/Team.h>
#include <support2_p/BinderKeys.h>

#include "./StdIO.cpp"

#include <stdio.h>

namespace B {
namespace Support2 {

int32 BBinder::gBinderTLS(tls_allocate());
int32 BLooper::TLS(tls_allocate());

const IByteInput::ptr Stdin(new BKernelIStr(STDIN_FILENO));
const IByteOutput::ptr Stdout(new BKernelOStr(STDOUT_FILENO));
const IByteOutput::ptr Stderr(new BKernelOStr(STDERR_FILENO));

ITextOutput::ptr bout(new BTextOutput(Stdout));
ITextOutput::ptr berr(new BTextOutput(Stderr));
ITextOutput::ptr bser(new BTextOutput(SerialOutput));

const BValue IValueInput::descriptor(BValue::TypeInfo(typeid(IValueInput)));
const BValue IValueOutput::descriptor(BValue::TypeInfo(typeid(IValueOutput)));

const BValue IByteInput::descriptor(BValue::TypeInfo(typeid(IByteInput)));
const BValue IByteOutput::descriptor(BValue::TypeInfo(typeid(IByteOutput)));
const BValue IByteSeekable::descriptor(BValue::TypeInfo(typeid(IByteSeekable)));


class init_main_thread { public: init_main_thread() {
	BLooper::InitMain();
} };

static init_main_thread _init_main_thread;

} // namespace Support2

namespace Private {
const BValue g_keyRead("Read");
const BValue g_keyWrite("Write");
const BValue g_keyEnd("End");
const BValue g_keySync("Sync");
const BValue g_keyPosition("Position");
const BValue g_keySeek("Seek");
const BValue g_keyMode("Mode");
const BValue g_keyPost("Post");
const BValue g_keyRedirectMessagesTo("RedirectMessagesTo");
const BValue g_keyPrint("Print");
const BValue g_keyBumpIndentLevel("BumpIndentLevel");
const BValue g_keyStatus("Status");
const BValue g_keyValue("Value");
const BValue g_keyWhich("Which");
const BValue g_keyFlags("Flags");
//const BValue g_key("");

const BValue g_keySysInspect(B_SYSTEM_TYPE,"inspect",8);
const BValue g_keySysInherit(B_SYSTEM_TYPE,"inherit",8);
const BValue g_keySysBequeath(B_SYSTEM_TYPE,"bequeath",9);

// IRender keys

} // namespace Private

} // namespace B
