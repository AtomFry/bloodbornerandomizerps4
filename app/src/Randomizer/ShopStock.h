// ShopStock.h - RANDOMIZE SHOP ARMOUR and RANDOMIZE SHOP ITEMS: the armour and
// the consumables the game's shops sell, each dealt back out at random across
// the listings that already sold that kind of thing.
//
// A SIBLING OF StartingWeapons, NOT AN EXTENSION OF IT. Both features write
// ShopLineupParam, so the field offsets and the never-touch list are shared
// through ShopLineup.h - but they share a table, not a feature. Feature 5 owns
// the equipType 0 rows and the five Hunter's Dream coffin slots; this owns the
// equipType 1 and equipType 3 rows, and never writes an equipType 0 listing.
// That three-way partition of the 1288 listings is the whole reason the two
// cannot interfere (spec 011 §4 F11).
//
// WHAT THE PLAYER SEES. The Bath Messengers' stock is scrambled: the armour on
// sale is 44 pieces dealt over 220 listings, and the consumables and materials
// are 57 items dealt over 397. Each listing keeps its OWN price, currency,
// purchase limit and shop - only the item id moves - so a Bold Hunter's Mark
// listing may now sell a rune at the mark's old price and limit. That is the
// feature, not a defect (spec 011 D3).
//
// THE THREE PROTECTED IDS. Blood Vials (1000), Quicksilver Bullets (900) and one
// dangling id (240) are neither sources nor destinations, matched on equipId in
// both the pool build and the write. A shop that stops selling vials ends a
// playthrough, which is why that list lives in ShopLineup.h and exists once.
//
// Like every other param feature this edits the decompressed item-data archive
// in place: one 4-byte integer over another inside an existing row, nothing
// resized, so the archive is never rebuilt. See Param/ParamBnd.h.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

struct ShopStockOptions {
    bool randomizeShopArmour = false;
    bool randomizeShopItems = false;

    bool Any() const { return randomizeShopArmour || randomizeShopItems; }
};

struct ShopStockResult {
    int armourRowsChanged = 0;
    int itemRowsChanged = 0;
};

// Applies whichever of the two buckets are enabled, in place inside `plain`
// (the decompressed archive). Returns false only on a structural problem, with
// `error` set; partial edits are possible in that case, so the caller should
// treat a failure as fatal to the run rather than continuing.
//
// Draws NOTHING and parses nothing when neither option is on, so an off run's
// roll sequence is exactly the one the app produced before this existed. With
// one on, that bucket's draws happen and the other bucket is not even walked.
bool RandomizeShopStock(std::vector<uint8_t>& plain,
                        const ParamMember& shopParam,
                        const ShopStockOptions& options,
                        std::mt19937& rng,
                        ShopStockResult& result,
                        std::string* error);

} // namespace bbr
