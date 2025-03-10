#include "IWidget.hpp"
#include "../../helpers/Log.hpp"
#include "../../helpers/VarList.hpp"
#include "../../helpers/MiscFunctions.hpp"
#include <chrono>
#include <unistd.h>

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 190100
#pragma comment(lib, "date-tz")
#include <date/tz.h>
namespace std {
    namespace chrono {
        using date::current_zone;
    }
}
#endif

Vector2D IWidget::posFromHVAlign(const Vector2D& viewport, const Vector2D& size, const Vector2D& offset, const std::string& halign, const std::string& valign) {
    Vector2D pos = offset;
    if (halign == "center")
        pos.x += viewport.x / 2.0 - size.x / 2.0;
    else if (halign == "left")
        pos.x += 0;
    else if (halign == "right")
        pos.x += viewport.x - size.x;
    else if (halign != "none")
        Debug::log(ERR, "IWidget: invalid halign {}", halign);

    if (valign == "center")
        pos.y += viewport.y / 2.0 - size.y / 2.0;
    else if (valign == "top")
        pos.y += viewport.y - size.y;
    else if (valign == "bottom")
        pos.y += size.y;
    else if (valign != "none")
        Debug::log(ERR, "IWidget: invalid halign {}", halign);

    return pos;
}

static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty())
        return;
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

static std::string getTime() {
    const auto current_zone = std::chrono::current_zone();
    const auto HHMMSS       = std::chrono::hh_mm_ss{current_zone->to_local(std::chrono::system_clock::now()) -
                                              std::chrono::floor<std::chrono::days>(current_zone->to_local(std::chrono::system_clock::now()))};
    const auto HRS          = HHMMSS.hours().count();
    const auto MINS         = HHMMSS.minutes().count();
    return (HRS < 10 ? "0" : "") + std::to_string(HRS) + ":" + (MINS < 10 ? "0" : "") + std::to_string(MINS);
}

IWidget::SFormatResult IWidget::formatString(std::string in) {

    auto username = getUserName();

    if (!username)
        Debug::log(ERR, "Error in formatString, username null. Errno: ", errno);

    IWidget::SFormatResult result;
    replaceAll(in, "$USER", username.value_or(""));
    replaceAll(in, "<br/>", std::string{"\n"});

    if (in.contains("$TIME")) {
        replaceAll(in, "$TIME", getTime());
        result.updateEveryMs = result.updateEveryMs != 0 && result.updateEveryMs < 1000 ? result.updateEveryMs : 1000;
    }

    if (in.starts_with("cmd[") && in.contains("]")) {
        // this is a command
        CVarList vars(in.substr(4, in.find_first_of(']') - 4));

        for (const auto& v : vars) {
            if (v.starts_with("update:")) {
                try {
                    result.updateEveryMs = std::stoull(v.substr(7));
                } catch (std::exception& e) { Debug::log(ERR, "Error parsing {} in cmd[]", v); }
            } else {
                Debug::log(ERR, "Unknown prop in string format {}", v);
            }
        }

        result.alwaysUpdate = true;
        in                  = in.substr(in.find_first_of(']') + 1);
        result.cmd          = true;
    }

    result.formatted = in;
    return result;
}
