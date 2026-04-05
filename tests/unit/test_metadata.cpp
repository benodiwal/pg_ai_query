#include <cassert>
#include "/home/lucifer/Desktop/pg_ai_query/src/core/metadata.hpp"

int main() {
    std::string text = "Use ST_Distance in PostGIS";

    auto sources = extractSources(text);
    assert(!sources.empty());

    double conf = calculateConfidence(text);
    assert(conf > 0);

    auto tags = extractTags(text);
    assert(!tags.empty());

    return 0;
}