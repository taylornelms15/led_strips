#include <linux/types.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>

/*
So, can run 300ns-on, 700ns-off, total bit time of 1us, so 1MHz frequency
Nah, 3 clocks per tick (?), so 333ns-on, 667ns-off
Ok, idea *was*: 0 == "1 0 0", 1 == "1 1 0"

I don't understand this fucker's conversion table, not really? it's tricky.
*/

static void write_bit_component(__u8 *out, int *written_bytes, __u8 *bit_offset, bool val)
{
	if (val)
		out[*written_bytes] |= (0x01 << (7 - (*bit_offset)++));
	else
		out[*written_bytes] &= ~(0x01 << (7 - (*bit_offset)++));

	if (*bit_offset == 8) {
		*bit_offset = 0;
		(*written_bytes)++;
	}
}

/**
 * convert_brightnesses_to_bitfields() - Converts raw LED brightnesses to bits to send
 * @out: binary data to send, converted for PWM engine
 * @vals: LED brightnesses to write
 * @num_vals: number of LED values. Must be multiple of 3.
 *
 * Return: 0 on success, negative value on error
 */
int convert_brightnesses_to_bitfields(__u8 *out, __u8 *vals, int num_vals)
{
	int i = 0;
	int written_bytes = 0;
	__u8 bit_offset = 0;

/*
	if (num_vals % 3 != 0)
		return -EINVAL;
		*/

	for(i = 0; i < num_vals; ++i) {
		__u8 j;
		for (j = 0; j < 8; ++j) {
			bool is_one = (0x01 << (7 - j)) & vals[i];
			
			write_bit_component(out, &written_bytes, &bit_offset, true);
			write_bit_component(out, &written_bytes, &bit_offset, is_one);
			write_bit_component(out, &written_bytes, &bit_offset, false);
		}//for each bit in input
	}//for each brightness


	return 0;
}

int main(int argc, char** argv)
{
	int i = 0;

	__u8 in_vals[256] = {0};
	__u8 out_vals[256 * 3] = {0};
	
	for (i = 0; i < 256; ++i){
		in_vals[i] = i;
	}
	
	convert_brightnesses_to_bitfields(out_vals, in_vals, 256);

	for(i = 0; i < 256 * 3; ++i) {
		printf("%#02x, ", out_vals[i]);
		if (i % 12 == 11)
			printf("\n");
	}

	return 0;

}

