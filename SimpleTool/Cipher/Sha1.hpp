/*
	sha1 �����㷨
*/
#ifndef SIM_SHA1_HPP_
#define SIM_SHA1_HPP_
#include <stdio.h>
#define SIM_SHA1_SIZE 5
namespace sim
{
	class Sha1
	{
	public:

		Sha1()
		{
			Reset();
		}
		virtual ~Sha1() {};

		/*
		 *  Re-initialize the class
		 */
		void Reset()
		{
			Length_Low = 0;
			Length_High = 0;
			Message_Block_Index = 0;

			H[0] = 0x67452301;
			H[1] = 0xEFCDAB89;
			H[2] = 0x98BADCFE;
			H[3] = 0x10325476;
			H[4] = 0xC3D2E1F0;

			Computed = false;
			Corrupted = false;
		}

		/*
		 *  Returns the message digest
		 */
		bool Result(unsigned message_digest_array[SIM_SHA1_SIZE])
		{
			int i;                                  // Counter

			if (Corrupted)
			{
				return false;
			}

			if (!Computed)
			{
				PadMessage();
				Computed = true;
			}

			for (i = 0; i < SIM_SHA1_SIZE; i++)
			{
				message_digest_array[i] = H[i];
			}

			return true;
		}

		int PrintResult(char * buff, unsigned buff_len)
		{
			//4*5= 20
			if (NULL == buff || buff_len < 20)
				return -1;

			unsigned message_digest_array[SIM_SHA1_SIZE] = {0};
			if (!Result(message_digest_array))
			{
				return -1;
			}
			return snprintf(buff, buff_len, "%x%x%x%x%x", message_digest_array[0]
				, message_digest_array[1]
				, message_digest_array[2]
				, message_digest_array[3]
				, message_digest_array[4]);
			
		}

		/*
		 *  Provide input to SHA1
		 */
		void Input(const unsigned char *message_array,
			unsigned            length)
		{
			if (!length)
			{
				return;
			}

			if (Computed || Corrupted)
			{
				Corrupted = true;
				return;
			}

			while (length-- && !Corrupted)
			{
				if (Message_Block_Index == 64)
				{
					ProcessMessageBlock();
				}

				Message_Block[Message_Block_Index++] = (*message_array & 0xFF);

				Length_Low += 8;
				Length_Low &= 0xFFFFFFFF;               // Force it to 32 bits
				if (Length_Low == 0)
				{
					Length_High++;
					Length_High &= 0xFFFFFFFF;          // Force it to 32 bits
					if (Length_High == 0)
					{
						Corrupted = true;               // Message is too long
					}
				}

				message_array++;
			}

			if (Message_Block_Index == 64)
			{
				ProcessMessageBlock();
			}
		}
		void Input(const char *message_array,
			unsigned            length)
		{
			Input((const unsigned char *)message_array, length);
		}

		static int SHA1(const char *message_array, unsigned length, char * sha1, unsigned sha1_len)
		{
			Sha1 t_sha1;
			t_sha1.Input(message_array, length);
			return t_sha1.PrintResult(sha1, sha1_len);
		}
	private:

		/*
		 *  Process the next 512 bits of the message
		 */
		void ProcessMessageBlock()
		{
			const unsigned K[] = {               // Constants defined for SHA-1
							   0x5A827999,
							   0x6ED9EBA1,
							   0x8F1BBCDC,
							   0xCA62C1D6
			};
			int         t;                          // Loop counter
			unsigned    temp;                       // Temporary word value
			unsigned    W[80];                      // Word sequence
			unsigned    A, B, C, D, E;              // Word buffers

			/*
			 *  Initialize the first 16 words in the array W
			 */
			for (t = 0; t < 16; t++)
			{
				W[t] = ((unsigned)Message_Block[t * 4]) << 24;
				W[t] |= ((unsigned)Message_Block[t * 4 + 1]) << 16;
				W[t] |= ((unsigned)Message_Block[t * 4 + 2]) << 8;
				W[t] |= ((unsigned)Message_Block[t * 4 + 3]);
			}

			for (t = 16; t < 80; t++)
			{
				W[t] = CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
			}

			A = H[0];
			B = H[1];
			C = H[2];
			D = H[3];
			E = H[4];

			for (t = 0; t < 20; t++)
			{
				temp = CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
				temp &= 0xFFFFFFFF;
				E = D;
				D = C;
				C = CircularShift(30, B);
				B = A;
				A = temp;
			}

			for (t = 20; t < 40; t++)
			{
				temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
				temp &= 0xFFFFFFFF;
				E = D;
				D = C;
				C = CircularShift(30, B);
				B = A;
				A = temp;
			}

			for (t = 40; t < 60; t++)
			{
				temp = CircularShift(5, A) +
					((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
				temp &= 0xFFFFFFFF;
				E = D;
				D = C;
				C = CircularShift(30, B);
				B = A;
				A = temp;
			}

			for (t = 60; t < 80; t++)
			{
				temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
				temp &= 0xFFFFFFFF;
				E = D;
				D = C;
				C = CircularShift(30, B);
				B = A;
				A = temp;
			}

			H[0] = (H[0] + A) & 0xFFFFFFFF;
			H[1] = (H[1] + B) & 0xFFFFFFFF;
			H[2] = (H[2] + C) & 0xFFFFFFFF;
			H[3] = (H[3] + D) & 0xFFFFFFFF;
			H[4] = (H[4] + E) & 0xFFFFFFFF;

			Message_Block_Index = 0;
		}

		/*
		 *  Pads the current message block to 512 bits
		 */
		void PadMessage()
		{
			/*
	*  Check to see if the current message block is too small to hold
	*  the initial padding bits and length.  If so, we will pad the
	*  block, process it, and then continue padding into a second block.
	*/
			if (Message_Block_Index > 55)
			{
				Message_Block[Message_Block_Index++] = 0x80;
				while (Message_Block_Index < 64)
				{
					Message_Block[Message_Block_Index++] = 0;
				}

				ProcessMessageBlock();

				while (Message_Block_Index < 56)
				{
					Message_Block[Message_Block_Index++] = 0;
				}
			}
			else
			{
				Message_Block[Message_Block_Index++] = 0x80;
				while (Message_Block_Index < 56)
				{
					Message_Block[Message_Block_Index++] = 0;
				}

			}

			/*
			 *  Store the message length as the last 8 octets
			 */
			Message_Block[56] = (Length_High >> 24) & 0xFF;
			Message_Block[57] = (Length_High >> 16) & 0xFF;
			Message_Block[58] = (Length_High >> 8) & 0xFF;
			Message_Block[59] = (Length_High) & 0xFF;
			Message_Block[60] = (Length_Low >> 24) & 0xFF;
			Message_Block[61] = (Length_Low >> 16) & 0xFF;
			Message_Block[62] = (Length_Low >> 8) & 0xFF;
			Message_Block[63] = (Length_Low) & 0xFF;

			ProcessMessageBlock();
		}

		/*
		 *  Performs a circular left shift operation
		 */
		inline unsigned CircularShift(int bits, unsigned word)
		{
			return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32 - bits));
		}
	private:
		unsigned H[SIM_SHA1_SIZE];                      // Message digest buffers

		unsigned Length_Low;                // Message length in bits
		unsigned Length_High;               // Message length in bits

		unsigned char Message_Block[64];    // 512-bit message blocks
		int Message_Block_Index;            // Index into message block array

		bool Computed;                      // Is the digest computed?
		bool Corrupted;                     // Is the message digest corruped?

	};
}
#endif