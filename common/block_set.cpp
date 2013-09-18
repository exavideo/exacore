/*
 * Copyright 2011, 2012, 2013 Exavideo LLC.
 * 
 * This file is part of openreplay.
 * 
 * openreplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * openreplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with openreplay.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "block_set.h"
#include "posix_util.h"
#include <string.h>
#include <stdexcept>

BlockSet::BlockSet( ) {
    fd = -1;
}

BlockSet::~BlockSet( ) {

}

void BlockSet::add_block(const char *label, void *data, size_t size) {
    BlockData blkdata;

    if (have_block(label)) {
        throw std::runtime_error("already got that block once");
    }

    memset(&blkdata, 0, sizeof(blkdata));

    strncpy(blkdata.label, label, sizeof(blkdata.label));
    blkdata.size = size;
    blkdata.data = data;
    blkdata.offset = -1;

    blocks.push_back(blkdata);
}

void BlockSet::add_block(const char *label, SerializeStream *sstr) {
    BlockData blkdata;

    if (have_block(label)) {
        throw std::runtime_error("already got that block once");
    }

    memset(&blkdata, 0, sizeof(blkdata));

    strncpy(blkdata.label, label, sizeof(blkdata.label));
    blkdata.size = sstr->bytes( );
    blkdata.sstr = sstr;
    blkdata.offset = -1;

    blocks.push_back(blkdata);
}

bool BlockSet::have_block(const char *label) {
    /* check if block with same label already in set */
    for (BlockData &blk : blocks) {
        if (strncmp(label, blk.label, 8) == 0) {
            return true;
        }
    }

    return false;
}

size_t BlockSet::write_all(int fd) const {
    size_t total_size = 0;
    std::list<BlockHeader> headers;
    BlockHeader hd;

    /* pass 1: collate all headers */
    for (const BlockData &blk : blocks) {
        strncpy(hd.label, blk.label, sizeof(hd.label));
        hd.size = blk.size;
        headers.push_back(hd);
    }

    /* terminate header list with a null label */
    memset(&hd, 0, sizeof(hd));
    headers.push_back(hd);
    
    /* pass 2: write all headers */
    for (BlockHeader &h : headers) {
        if (::write_all(fd, &h, sizeof(h)) <= 0) {
            throw POSIXError("write_all");
        }

        total_size += sizeof(h);
    }

    /* pass 3: write all data blocks */
    for (const BlockData &blk : blocks) {
        if (blk.data) {
            if (::write_all(fd, blk.data, blk.size) <= 0) {
                throw POSIXError("write_all");
            }
        } else if (blk.sstr) {
            blk.sstr->writeout(fd);
        }

        total_size += blk.size;
    }

    return total_size;
}

void BlockSet::begin_read(int fd, off_t start) {
    /* read in all block headers (but no data yet) */
    BlockHeader hd;
    BlockData data;
    off_t hdbase = start;

    if (!blocks.empty( )) {
        throw std::runtime_error("read into non-empty BlockSet");
    }

    /* read all headers */
    do {
        if (pread_all(fd, &hd, sizeof(hd), hdbase) <= 0) {
            throw POSIXError("pread_all");
        }
        strncpy(data.label, hd.label, sizeof(hd.label));
        data.size = hd.size;
        data.data = NULL;
        data.offset = -1;
        blocks.push_back(data);

        hdbase += sizeof(hd);
    } while (hd.label[0] != '\0');

    /* 
     * now hdbase is the offset of the first data block 
     * we iterate through all blocks and compute offsets
     * based on this one.
     */
    for (BlockData &blk : blocks) {
        blk.offset = hdbase;
        hdbase += blk.size;
    }

    this->fd = fd;
}

BlockSet::BlockData &BlockSet::find_block(const char *label) {
    for (BlockData &blk : blocks) {
        if (strncmp(label, blk.label, sizeof(blk.label)) == 0) {
            return blk;
        }
    }
    
    /* throw BlockNotFoundError? */
    throw std::runtime_error("Block not found");
}

size_t BlockSet::read_data(const char *label, void *data, size_t size) {
    BlockData &blk = find_block(label);
    size_t size_to_read = std::min(size, blk.size);

    if (fd == -1) {
        throw std::runtime_error("begin_read was not called yet");
    }

    if (pread_all(fd, data, size_to_read, blk.offset) <= 0) {
        throw std::runtime_error("pread_all");
    }

    return size_to_read;
}

off_t BlockSet::end_offset( ) {
    return blocks.back( ).offset;
}
