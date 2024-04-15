#include "seed.h"

static u16 rotateLeft(u16 seed, u8 count)
{
    u32 tmp = seed;

    for (u32 i = 0; i < count; i++)
    {
        tmp <<= 1;
        if (tmp & 0x10000)
            tmp |= 1;
    }

    return tmp&0xffff;
}

static u16 rotateRight(u16 seed, u8 count)
{
    u32 tmp = seed << 16;

    for (u32 i = 0; i < count; i++)
    {
        tmp >>= 1;
        if (tmp & 0x8000)
            tmp |= (1 << 31);
    }

    tmp >>= 16;

    return tmp&0xffff;
}

static u16 oneOperation(const u8 *algoptr, u16 seed)
{
    u8 cmd = *algoptr++;

    switch (cmd)
    {
    // Not in the old list
    case 0x05: // Rotate left 8, always
        return rotateLeft(seed, 8);
    case 0x37: // AND
        return seed & (algoptr[1] << 8 | algoptr[0]);
    case 0x52: // OR
        return seed | (algoptr[1] << 8 | algoptr[0]);
    case 0x75: // Add
        return seed + (algoptr[1] << 8 | algoptr[0]);
    case 0xf8: // Subtract
        return seed - (u16)(algoptr[1] << 8 | algoptr[0]);

    case 0x14: // Add
        return seed + (algoptr[0] << 8 | algoptr[1]);
    case 0x2a: // Complement
        if (algoptr[0] < algoptr[1]) return ~seed + 1;
        else /* * * * * * * * * * */ return ~seed;
    case 0x4c: // Rotate left
        return rotateLeft(seed, algoptr[0]); // Use HH
    case 0x6b: // Rotate right
        return rotateRight(seed, algoptr[1]); // Use LL
    case 0x7e: // Rotate, add
        seed = (seed << 8 | seed >> 8);
        if (algoptr[1] > algoptr[0]) return seed + (u16)(algoptr[1] << 8 | algoptr[0]);
        else /* * * * * * * * * * */ return seed + (u16)(algoptr[0] << 8 | algoptr[1]);
    case 0x98: // Subtract
        return seed - (u16)(algoptr[0] << 8 | algoptr[1]);

    default:
        // printf("Unk algo: %02x\n", cmd);
        break;
    }

    return seed;
}

static u16 calcKey(const u8 *array, u16 seed)
{
    const u8 *algoptr = &array[1];
    seed = oneOperation(&algoptr[0], seed);
    seed = oneOperation(&algoptr[3], seed);
    seed = oneOperation(&algoptr[6], seed);
    return oneOperation(&algoptr[9], seed);
}

static const void *matchedpairs[] = {
    "ng9-3, Motronic me9.6",
    me96pairs,

    "ng9-5, AcDelco e39",
    delcoe39_ng95,

    "Unknown, AcDelco e39A",
    delcoe39A_unknown,

    "??, AcDelco e78",
    delcoe78,

    // "9-5, Trionic 7 type 0",
    // trionic7type0,
    // "9-5, Trionic 7 type 1",
    // trionic7type1,
    "ng9-3, Trionic 8, type normal",
    trionic8_normal,

    "ng9-3, Trionic 8, type fb",
    trionic8_fb,
    "ng9-3, Trionic 8, type fd",
    trionic8_fd,
    "ng9-3, CIM",
    trionic8_CIM,
};

int main()
{
    for (u32 list = 0; list < ((sizeof(matchedpairs) / sizeof(matchedpairs[0]))/2); list++)
    {
        u8  *name  =  (u8 *) matchedpairs[list*2];
        u16  count = *(u16*) matchedpairs[(list*2)+1];
        u16 *array = &((u16*) matchedpairs[(list*2)+1])[2];
        printf("\n\"%s\"\n", name);

        for (u32 set = 0; set < count; set++)
        {
            u16 seed  = array[(set*2)];
            u16 key   = array[(set*2)+1];
            u16 match = 0;

            printf("Pair %u.. ", set);

            for (u32 i = 0; i < (sizeof(oldArray) / sizeof(oldArray[0])); i++)
            {
                if (key == calcKey(&oldArray[i][0], seed))
                {
                    printf("matched with old set, algo %u\n", i);
                    match = 1;
                    break;
                }
            }

            if (!match)
            for (u32 i = 0; i < (sizeof(newArray_1) / sizeof(newArray_1[0])); i++)
            {
                if (key == calcKey(&newArray_1[i][0], seed))
                {
                    printf("matched with set 1, algo %u\n", i);
                    match = 1;
                    break;
                }
            }

            // These two could be other manufacturers..
            if (!match)
            for (u32 i = 0; i < (sizeof(newArray_2) / sizeof(newArray_2[0])); i++)
            {
                if (key == calcKey(&newArray_2[i][0], seed))
                {
                    printf("matched with set 2, algo %u\n", i);
                    match = 1;
                    break;
                }
            }

            if (!match)
            for (u32 i = 0; i < (sizeof(newArray_3) / sizeof(newArray_3[0])); i++)
            {
                if (key == calcKey(&newArray_3[i][0], seed))
                {
                    printf("matched with set 3, algo %u\n", i);
                    match = 1;
                    break;
                }
            }

            if (!match)
                printf("has no matching algo\n");
        }
    }

    return 0;
}
