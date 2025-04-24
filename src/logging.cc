#include "logging.h"

#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace logging {
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
using severity_level = boost::log::trivial::severity_level;

void init_logging(std::ostream* console_stream,
                  const std::string& file_pattern) {
  // Remove any existing sinks (useful for tests that re-init)
  auto core = boost::log::core::get();
  core->remove_all_sinks();

  // common formatter
  auto fmt = expr::format("[%1%] [%2%] <%3%> %4%") %
             expr::format_date_time<boost::posix_time::ptime>(
                 "TimeStamp", "%Y-%m-%d %H:%M:%S.%f") %
             expr::attr<boost::log::attributes::current_thread_id::value_type>(
                 "ThreadID") %
             expr::attr<boost::log::trivial::severity_level>("Severity") %
             expr::smessage;

  // Console sink (Error and above)
  if (console_stream) {
    typedef sinks::synchronous_sink<sinks::text_ostream_backend> ostream_sink;
    auto sink = boost::make_shared<ostream_sink>();
    sink->locked_backend()->add_stream(
        boost::shared_ptr<std::ostream>(console_stream, [](void*) {}));
    sink->set_formatter(fmt);
    // filter: only errors and above to console
    sink->set_filter(expr::attr<severity_level>("Severity") >= severity_level::error);
    core->add_sink(sink);
  }

  // File sink （Info and above）
  if (!file_pattern.empty()) {
    typedef sinks::synchronous_sink<sinks::text_file_backend> file_sink;
    auto sink = boost::make_shared<file_sink>(
        keywords::file_name = file_pattern,
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation =
            sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::auto_flush = true);
    sink->set_formatter(fmt);
    // filter: only info and above to file
    sink->set_filter(expr::attr<severity_level>("Severity") >= severity_level::info);
    core->add_sink(sink);
  }

  // Common attributes (TimeStamp, ThreadID, etc.)
  boost::log::add_common_attributes();
}

}  // namespace logging
