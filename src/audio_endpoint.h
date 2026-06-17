#pragma once

namespace wingnome {

class AudioEndpoint {
public:
    static AudioEndpoint& get();

    bool read(float& level, bool& muted);
    bool write(float level, bool muted);
};

}  // namespace wingnome
