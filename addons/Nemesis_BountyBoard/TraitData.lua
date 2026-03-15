-- TraitData.lua
-- Bitmask lookup tables for nemesis origin and earned traits.
-- Bit values must stay in sync with NemesisEliteConstants.h on the server.

NemesisBountyBoard_TraitData = {}

NemesisBountyBoard_TraitData.OriginTraits = {
    [0x01] = { name = "Ambusher",     color = "FFA500" },
    [0x02] = { name = "Executioner",  color = "CC3333" },
    [0x04] = { name = "Mage Bane",    color = "6699FF" },
    [0x08] = { name = "Healer Bane",  color = "33CC33" },
    [0x10] = { name = "Giant Slayer", color = "FFD700" },
    [0x20] = { name = "Opportunist",  color = "888888" },
    [0x40] = { name = "Umbralforged", color = "9D69DE" },
    [0x80] = { name = "Empowered",    color = "FF4444" },
}

NemesisBountyBoard_TraitData.EarnedTraits = {
    [0x01] = { name = "Coward",      color = "AAAA00" },
    [0x02] = { name = "Notorious",   color = "FF6600" },
    [0x04] = { name = "Survivor",    color = "66CCFF" },
    [0x08] = { name = "Territorial", color = "33AA33" },
}

-- DecodeBitmask: returns a list of {name, color} for every set bit in value.
-- Iterates bits from lowest to highest for stable ordering.
function NemesisBountyBoard_TraitData.DecodeBitmask(value, traitTable)
    local result = {}
    local bit = 1
    while bit <= 0xFF do
        if math.floor(value / bit) % 2 == 1 then
            local entry = traitTable[bit]
            if entry then
                table.insert(result, entry)
            end
        end
        bit = bit * 2
    end
    return result
end

-- FormatTraitList: returns a colored, comma-separated string from a decoded trait list.
-- Returns gray "None" when the list is empty.
function NemesisBountyBoard_TraitData.FormatTraitList(traits)
    if not traits or #traits == 0 then
        return "|cFFAAAAAA None|r"
    end
    local parts = {}
    for _, trait in ipairs(traits) do
        table.insert(parts, "|cFF" .. trait.color .. trait.name .. "|r")
    end
    return table.concat(parts, ", ")
end
