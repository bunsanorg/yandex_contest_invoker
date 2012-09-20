/*!
 * \file
 *
 * \brief Control process implementation.
 *
 * This binary should not be used directly.
 * Use yandex::contest::invoker::detail::execution::AsyncProcessGroup
 * or cli instead.
 *
 * \see yandex::contest::invoker::detail::execution::AsyncProcessGroup
 * \see yandex::contest::invoker::ControlProcessSettings
 */

#include "yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp"

#include "yandex/contest/SerializationCast.hpp"

#include "yandex/contest/system/Trace.hpp"

#include <iostream>

int main()
{
    try
    {
        yandex::contest::system::Trace::handle(SIGABRT);
        yandex::contest::system::Trace::handle(SIGSEGV);
        using yandex::contest::invoker::detail::execution::AsyncProcessGroup;
        using namespace yandex::contest::serialization;
        AsyncProcessGroup::Task task;
        BinaryReader::readFromStream(std::cin, task);
        BinaryWriter::writeToStream(std::cout, AsyncProcessGroup::execute(task));
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
