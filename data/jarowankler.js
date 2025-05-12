// downloaded from https://unpkg.com/@skyra/jaro-winkler@1.1.1/dist/iife/index.global.js and adapted
'use strict';

var __defProp = Object.defineProperty;
var __name = (target, value) => __defProp(target, "name", {value, configurable: true});

// src/index.ts
function decodeUtf8(str) {
    return typeof str === "string" ? [...str] : str;
}

__name(decodeUtf8, "decodeUtf8");

function jaroWinkler(stringCompare, stringCompareWith) {
    const a1 = decodeUtf8(stringCompare);
    const a2 = decodeUtf8(stringCompareWith);
    const l1 = a1.length;
    const l2 = a2.length;
    const matches1 = new Uint8Array(l1);
    const matches2 = new Uint8Array(l2);
    const matches = getMatching(a1, a2, matches1, matches2);
    if (matches <= 0)
        return 0;
    const transpositions = getTranspositions(a1, a2, matches1, matches2);
    const similarity = (matches / l1 + matches / l2 + (matches - transpositions) / matches) / 3;
    const prefixScale = 0.1;
    const prefix = getPrefix(a1, a2);
    return similarity + prefix * prefixScale * (1 - similarity);
}

__name(jaroWinkler, "jaroWinkler");

function getMatching(a1, a2, matches1, matches2) {
    const matchWindow = Math.floor(Math.max(a1.length, a2.length) / 2);
    let matches = 0;
    let index1 = 0;
    let index2 = 0;
    for (index1 = 0; index1 < a1.length; index1++) {
        const start = Math.max(0, index1 - matchWindow);
        const end = Math.min(index1 + matchWindow + 1, a2.length);
        for (index2 = start; index2 < end; index2++) {
            if (matches2[index2]) {
                continue;
            }
            if (a1[index1] !== a2[index2]) {
                continue;
            }
            matches1[index1] = 1;
            matches2[index2] = 1;
            ++matches;
            break;
        }
    }
    return matches;
}

__name(getMatching, "getMatching");

function getTranspositions(a1, a2, matches1, matches2) {
    let transpositions = 0;
    for (let i1 = 0, i2 = 0; i1 < a1.length; i1++) {
        if (matches1[i1] === 0)
            continue;
        while (i2 < matches2.length && matches2[i2] === 0)
            i2++;
        if (a1[i1] !== a2[i2])
            transpositions++;
        i2++;
    }
    return Math.floor(transpositions / 2);
}

__name(getTranspositions, "getTranspositions");

function getPrefix(a1, a2) {
    const prefixLimit = 4;
    let p = 0;
    for (; p < prefixLimit; p++) {
        if (a1[p] !== a2[p])
            return p;
    }
    return ++p;
}
