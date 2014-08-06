#include "PlainTextEventWriter.hpp"

#include <boost/format.hpp>
#include <boost/io/detail/quoted_manip.hpp>
#include <boost/variant/static_visitor.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    PlainTextEventWriter::PlainTextEventWriter(Connection &connection):
        connection_(connection),
        writer_(connection_) {}

    void PlainTextEventWriter::write(const notifier::Event &event)
    {
        struct EventConverter: boost::static_visitor<std::string>
        {
            std::string operator()(const notifier::SpawnEvent &event) const
            {
                return str(
                    boost::format("spawn "
                                  "meta.id %1% "
                                  "meta.name %2%") %
                        event.meta.id %
                        boost::io::quoted(event.meta.name)
                );
            }
            std::string operator()(const notifier::TerminationEvent &event) const
            {
                return str(
                    boost::format("termination "
                                  "meta.id %1% "
                                  "meta.name %2% "
                                  "result.completionStatus %3%") %
                        event.meta.id %
                        boost::io::quoted(event.meta.name) %
                        event.result.completionStatus
                );
            }
        };
        writer_.write(boost::apply_visitor(EventConverter(), event));
    }

    void PlainTextEventWriter::close()
    {
        writer_.close();
    }
}}}}}}
