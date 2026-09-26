#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# TickMath translation: Uniswap v3-core 1.0.1, contracts/libraries/TickMath.sol.
# Offline sizing only; no swaps, bridge messages or transactions are submitted.
import argparse
import json
from decimal import Decimal, localcontext
from fractions import Fraction
from math import isqrt

Q96 = 1 << 96
FACTORS = (340265354078544963557816517032075149313, 340248342086729790484326174814286782778, 340214320654664324051920982716015181260, 340146287995602323631171512101879684304, 340010263488231146823593991679159461444, 339738377640345403697157401104375502016, 339195258003219555707034227454543997025, 338111622100601834656805679988414885971, 335954724994790223023589805789778977700, 331682121138379247127172139078559817300, 323299236684853023288211250268160618739, 307163716377032989948697243942600083929, 277268403626896220162999269216087595045, 225923453940442621947126027127485391333, 149997214084966997727330242082538205943, 66119101136024775622716233608466517926, 12847376061809297530290974190478138313, 485053260817066172746253684029974020, 691415978906521570653435304214168, 1404880482679654955896180642)

def ceildiv(a, b):
    return (a + b - 1) // b

def sqrt_at_tick(tick):
    if not -887272 <= tick <= 887272:
        raise ValueError("tick outside Uniswap V3 bounds")
    ratio = 1 << 128
    for bit, factor in enumerate(FACTORS):
        if abs(tick) & (1 << bit):
            ratio = ratio * factor >> 128
    if tick > 0:
        ratio = ((1 << 256) - 1) // ratio
    return ceildiv(ratio, 1 << 32)

def tick_at_sqrt(p):
    if not sqrt_at_tick(-887272) <= p < sqrt_at_tick(887272):
        raise ValueError("sqrtPriceX96 outside pool bounds")
    lo, hi = -887272, 887272
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if sqrt_at_tick(mid) <= p: lo = mid
        else: hi = mid
    return lo

def amounts(liquidity, p, a, b):
    # Mint amounts round UP, unlike the periphery withdrawal quotation.
    x = min(max(p, a), b)
    return (ceildiv((liquidity << 96) * (b-x), b*x),
            ceildiv(liquidity * (x-a), Q96))

def size(quote_raw, p, lower, upper, spacing, quote_index):
    if spacing <= 0 or lower >= upper or lower % spacing or upper % spacing:
        raise ValueError("ordered ticks must be multiples of positive spacing")
    a,b = sqrt_at_tick(lower),sqrt_at_tick(upper)
    tick_at_sqrt(p)
    if not a < p < b:
        raise ValueError("graduation price must be strictly inside the active range")
    if not 0 < quote_raw < 1 << 256:
        raise ValueError("quote budget must fit uint256 and be positive")
    # Match periphery liquidity rounding for the limiting quote token.
    liquidity = (quote_raw * Q96 // (p-a) if quote_index == 1 else
                 quote_raw * (p*b // Q96) // (b-p))
    if not 0 < liquidity < 1 << 128:
        raise ValueError("liquidity must fit uint128 and be positive")
    pair = amounts(liquidity,p,a,b)
    if pair[quote_index] > quote_raw or max(pair) >= 1 << 256:
        raise ValueError("amount overflow or budget exceeded")
    return liquidity,pair

def human(raw, decimals):
    with localcontext() as ctx:
        ctx.prec = 100
        return format(Decimal(raw) / Decimal(10)**decimals, 'f')

def main():
    parser=argparse.ArgumentParser(description="Size allocation C for a V3 position; offline only")
    parser.add_argument('--usdc',required=True,help="human USDC budget, at most 6 decimals")
    pricing=parser.add_mutually_exclusive_group(required=True)
    pricing.add_argument('--price',help="USDC per NANO; synthetic scenario, not a market prediction")
    pricing.add_argument('--sqrt-price-x96',type=int,help="actual pool slot0 value")
    parser.add_argument('--quote-index',type=int,choices=[0,1],required=True,help="USDC token index determined by address ordering")
    parser.add_argument('--token-decimals',type=int,default=18)
    parser.add_argument('--tick-lower',type=int,required=True)
    parser.add_argument('--tick-upper',type=int,required=True)
    parser.add_argument('--tick-spacing',type=int,required=True,help="read from the actual pool/factory")
    args=parser.parse_args()
    if not 0 <= args.token_decimals <= 36: raise ValueError("unsupported token decimals")
    quote=Fraction(args.usdc)*10**6
    if quote.denominator != 1: raise ValueError("USDC supports six decimals")
    if args.price:
        price=Fraction(args.price)
        if price <= 0: raise ValueError("price must be positive")
        ratio=price*Fraction(10**6,10**args.token_decimals)
        if args.quote_index == 0: ratio=1/ratio
        p=isqrt((ratio.numerator << 192)//ratio.denominator)
    else: p=args.sqrt_price_x96
    liquidity,pair=size(int(quote),p,args.tick_lower,args.tick_upper,args.tick_spacing,args.quote_index)
    nano=pair[1-args.quote_index]
    print(json.dumps({"scope":"single-position mint sizing; no swap or price-movement prediction", "quoteIndex":args.quote_index,
      "sqrtPriceX96":str(p),"currentTick":tick_at_sqrt(p),"tickLower":args.tick_lower,"tickUpper":args.tick_upper,
      "liquidity":str(liquidity),"allocationCRaw":str(nano),"allocationCNano":human(nano,args.token_decimals),
      "usdcUsedRaw":str(pair[args.quote_index]),"usdcUsed":human(pair[args.quote_index],6),"usdcRemaining":human(int(quote)-pair[args.quote_index],6),
      "tickMovementFromAddingLiquidity":0},indent=2))

if __name__ == '__main__':
    main()
