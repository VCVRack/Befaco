

// cleaner sat16 by http://www.moseleyinstruments.com/
__attribute__((unused))
static int16_t sat16(int32_t n, int rshift) {
    // we should always round towards 0
    // to avoid recirculating round-off noise
    //
    // a 2s complement positive number is always
    // rounded down, so we only need to take
    // care of negative numbers
    if (n < 0) {
        n = n + (~(0xFFFFFFFFUL << rshift));
    }
    n = n >> rshift;
    if (n > 32767) {
        return 32767;
    }
    if (n < -32768) {
        return -32768;
    }
    return n;
}


class AudioEffectFreeverb : public AudioStream {
public:
	AudioEffectFreeverb() : AudioStream(1) {
		memset(comb1buf, 0, sizeof(comb1buf));
		memset(comb2buf, 0, sizeof(comb2buf));
		memset(comb3buf, 0, sizeof(comb3buf));
		memset(comb4buf, 0, sizeof(comb4buf));
		memset(comb5buf, 0, sizeof(comb5buf));
		memset(comb6buf, 0, sizeof(comb6buf));
		memset(comb7buf, 0, sizeof(comb7buf));
		memset(comb8buf, 0, sizeof(comb8buf));
		comb1index = 0;
		comb2index = 0;
		comb3index = 0;
		comb4index = 0;
		comb5index = 0;
		comb6index = 0;
		comb7index = 0;
		comb8index = 0;
		comb1filter = 0;
		comb2filter = 0;
		comb3filter = 0;
		comb4filter = 0;
		comb5filter = 0;
		comb6filter = 0;
		comb7filter = 0;
		comb8filter = 0;
		combdamp1 = 6553;
		combdamp2 = 26215;
		combfeeback = 27524;
		memset(allpass1buf, 0, sizeof(allpass1buf));
		memset(allpass2buf, 0, sizeof(allpass2buf));
		memset(allpass3buf, 0, sizeof(allpass3buf));
		memset(allpass4buf, 0, sizeof(allpass4buf));
		allpass1index = 0;
		allpass2index = 0;
		allpass3index = 0;
		allpass4index = 0;
	}

	void update(const audio_block_t* block, audio_block_t* outblock) {


		int i;
		int16_t input, bufout, output;
		int32_t sum;

		/*
		outblock = allocate();
		if (!outblock) {
			audio_block_t* tmp = receiveReadOnly(0);
			if (tmp)
				release(tmp);
			return;
		}
		block = receiveReadOnly(0);
		if (!block)
			block = &zeroblock;
			*/

		for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			// TODO: scale numerical range depending on roomsize & damping
			input = sat16(block->data[i] * 8738, 17); // for numerical headroom
			sum = 0;

			bufout = comb1buf[comb1index];
			sum += bufout;
			comb1filter = sat16(bufout * combdamp2 + comb1filter * combdamp1, 15);
			comb1buf[comb1index] = sat16(input + sat16(comb1filter * combfeeback, 15), 0);
			if (++comb1index >= sizeof(comb1buf) / sizeof(int16_t))
				comb1index = 0;

			bufout = comb2buf[comb2index];
			sum += bufout;
			comb2filter = sat16(bufout * combdamp2 + comb2filter * combdamp1, 15);
			comb2buf[comb2index] = sat16(input + sat16(comb2filter * combfeeback, 15), 0);
			if (++comb2index >= sizeof(comb2buf) / sizeof(int16_t))
				comb2index = 0;

			bufout = comb3buf[comb3index];
			sum += bufout;
			comb3filter = sat16(bufout * combdamp2 + comb3filter * combdamp1, 15);
			comb3buf[comb3index] = sat16(input + sat16(comb3filter * combfeeback, 15), 0);
			if (++comb3index >= sizeof(comb3buf) / sizeof(int16_t))
				comb3index = 0;

			bufout = comb4buf[comb4index];
			sum += bufout;
			comb4filter = sat16(bufout * combdamp2 + comb4filter * combdamp1, 15);
			comb4buf[comb4index] = sat16(input + sat16(comb4filter * combfeeback, 15), 0);
			if (++comb4index >= sizeof(comb4buf) / sizeof(int16_t))
				comb4index = 0;

			bufout = comb5buf[comb5index];
			sum += bufout;
			comb5filter = sat16(bufout * combdamp2 + comb5filter * combdamp1, 15);
			comb5buf[comb5index] = sat16(input + sat16(comb5filter * combfeeback, 15), 0);
			if (++comb5index >= sizeof(comb5buf) / sizeof(int16_t))
				comb5index = 0;

			bufout = comb6buf[comb6index];
			sum += bufout;
			comb6filter = sat16(bufout * combdamp2 + comb6filter * combdamp1, 15);
			comb6buf[comb6index] = sat16(input + sat16(comb6filter * combfeeback, 15), 0);
			if (++comb6index >= sizeof(comb6buf) / sizeof(int16_t))
				comb6index = 0;

			bufout = comb7buf[comb7index];
			sum += bufout;
			comb7filter = sat16(bufout * combdamp2 + comb7filter * combdamp1, 15);
			comb7buf[comb7index] = sat16(input + sat16(comb7filter * combfeeback, 15), 0);
			if (++comb7index >= sizeof(comb7buf) / sizeof(int16_t))
				comb7index = 0;

			bufout = comb8buf[comb8index];
			sum += bufout;
			comb8filter = sat16(bufout * combdamp2 + comb8filter * combdamp1, 15);
			comb8buf[comb8index] = sat16(input + sat16(comb8filter * combfeeback, 15), 0);
			if (++comb8index >= sizeof(comb8buf) / sizeof(int16_t))
				comb8index = 0;

			output = sat16(sum * 31457, 17);

			bufout = allpass1buf[allpass1index];
			allpass1buf[allpass1index] = output + (bufout >> 1);
			output = sat16(bufout - output, 1);
			if (++allpass1index >= sizeof(allpass1buf) / sizeof(int16_t))
				allpass1index = 0;

			bufout = allpass2buf[allpass2index];
			allpass2buf[allpass2index] = output + (bufout >> 1);
			output = sat16(bufout - output, 1);
			if (++allpass2index >= sizeof(allpass2buf) / sizeof(int16_t))
				allpass2index = 0;

			bufout = allpass3buf[allpass3index];
			allpass3buf[allpass3index] = output + (bufout >> 1);
			output = sat16(bufout - output, 1);
			if (++allpass3index >= sizeof(allpass3buf) / sizeof(int16_t))
				allpass3index = 0;

			bufout = allpass4buf[allpass4index];
			allpass4buf[allpass4index] = output + (bufout >> 1);
			output = sat16(bufout - output, 1);
			if (++allpass4index >= sizeof(allpass4buf) / sizeof(int16_t))
				allpass4index = 0;

			outblock->data[i] = sat16(output * 30, 0);
		}
		/*
		transmit(outblock);
		release(outblock);
		if (block != &zeroblock)
			release((audio_block_t*)block);
		*/


	}

	void roomsize(float n) {
		if (n > 1.0f)
			n = 1.0f;
		else if (n < 0.0f)
			n = 0.0f;
		combfeeback = (int)(n * 9175.04f) + 22937;
	}
	void damping(float n) {
		if (n > 1.0f)
			n = 1.0f;
		else if (n < 0.0f)
			n = 0.0f;
		int x1 = (int)(n * 13107.2f);
		int x2 = 32768 - x1;
		//__disable_irq();
		combdamp1 = x1;
		combdamp2 = x2;
		//__enable_irq();
	}
private:
	audio_block_t* inputQueueArray[1];
	int16_t comb1buf[1116];
	int16_t comb2buf[1188];
	int16_t comb3buf[1277];
	int16_t comb4buf[1356];
	int16_t comb5buf[1422];
	int16_t comb6buf[1491];
	int16_t comb7buf[1557];
	int16_t comb8buf[1617];
	uint16_t comb1index;
	uint16_t comb2index;
	uint16_t comb3index;
	uint16_t comb4index;
	uint16_t comb5index;
	uint16_t comb6index;
	uint16_t comb7index;
	uint16_t comb8index;
	int16_t comb1filter;
	int16_t comb2filter;
	int16_t comb3filter;
	int16_t comb4filter;
	int16_t comb5filter;
	int16_t comb6filter;
	int16_t comb7filter;
	int16_t comb8filter;
	int16_t combdamp1;
	int16_t combdamp2;
	int16_t combfeeback;
	int16_t allpass1buf[556];
	int16_t allpass2buf[441];
	int16_t allpass3buf[341];
	int16_t allpass4buf[225];
	uint16_t allpass1index;
	uint16_t allpass2index;
	uint16_t allpass3index;
	uint16_t allpass4index;
};

