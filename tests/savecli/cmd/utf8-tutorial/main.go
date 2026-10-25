// Offline conversion of the bundled ASCII tutorial, not a user-save importer.
package main

import (
	"encoding/binary"
	"fmt"
	"os"
	"stars-save/savefile"
)

func main() {
	if len(os.Args) != 3 {
		panic("usage: utf8-tutorial source destination")
	}
	data, err := os.ReadFile(os.Args[1])
	if err != nil {
		panic(err)
	}
	output := append([]byte(nil), data...)
	// All three resource streams begin with a turn count and end with a BOF sentinel.
	off := 1
	for turn := 0; turn < int(data[0]); turn++ {
		start := off
		off += 2 + int(binary.LittleEndian.Uint16(data[off:])&1023)
		for off+2 <= len(data) && binary.LittleEndian.Uint16(data[off:])>>10 != savefile.RtBOF {
			off += 2 + int(binary.LittleEndian.Uint16(data[off:])&1023)
		}
		if off > len(data) {
			panic("truncated tutorial turn")
		}
		records, err := savefile.ReadRecords(data[start:off], savefile.DefaultTables())
		if err != nil {
			panic(err)
		}
		bof, err := savefile.ParseBOF(records[0].Data)
		if err != nil {
			panic(err)
		}
		for i := 0; i < len(records); i++ {
			r := records[i]
			if r.Type != savefile.RtBtlData || bof.VerMinor >= 80 {
				continue
			}
			// UpdateBattleRecords upgrades 2.60 battle event headers. Continuation
			// framing and XOR stream lengths stay unchanged; patch by XOR deltas.
			original := append([]byte(nil), r.Data...)
			positions := make([]int, len(r.Data))
			for j := range positions {
				positions[j] = start + r.Offset + 2 + j
			}
			for i+1 < len(records) && records[i+1].Type == savefile.RtContinue {
				i++
				next := records[i]
				for j, b := range next.Data {
					original = append(original, b)
					positions = append(positions, start+next.Offset+2+j)
				}
			}
			updated := append([]byte(nil), original...)
			if len(updated) < 14 {
				panic("truncated battle")
			}
			size := int(binary.LittleEndian.Uint16(updated[6:]))
			if size != len(updated) {
				panic(fmt.Sprintf("battle length %d != %d", size, len(updated)))
			}
			for j := 14 + int(updated[3])*29; j < size; {
				if j+6 > size {
					panic("truncated battle event")
				}
				attacker, count := updated[j+2], updated[j+3]
				binary.LittleEndian.PutUint16(updated[j+2:], uint16(count))
				updated[j+5] = attacker
				j += 6 + int(count)*8
				if j > size {
					panic("truncated battle kills")
				}
			}
			for j := range updated {
				output[positions[j]] ^= original[j] ^ updated[j]
			}
		}
		version := binary.LittleEndian.Uint16(data[start+10:])
		binary.LittleEndian.PutUint16(output[start+10:], (version & ^uint16(0xfff))|85<<5)
	}
	if err := os.WriteFile(os.Args[2], output, 0644); err != nil {
		panic(err)
	}
}
