#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
  int fd, i;
  int16_t value[2];
  volatile uint32_t *slcr, *axi_hp0;
  volatile void *cfg, *ram;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  slcr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8000000);
  axi_hp0 = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8008000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  ram = mmap(NULL, 1024*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);

  // set FPGA clock to 143 MHz and HP0 bus width to 64 bits
  slcr[2] = 0xDF0D;
  slcr[92] = (slcr[92] & ~0x03F03F30) | 0x00100700;
  slcr[144] = 0;
  axi_hp0[0] &= ~1;
  axi_hp0[5] &= ~1;

  // reset writer
  *((uint32_t *)(cfg + 0)) &= ~4;
  *((uint32_t *)(cfg + 0)) |= 4;

  // reset fifo and filters
  *((uint32_t *)(cfg + 0)) &= ~1;
  *((uint32_t *)(cfg + 0)) |= 1;

  // wait 1 second
  sleep(1);

  // enter reset mode for packetizer
  *((uint32_t *)(cfg + 0)) &= ~2;

  // set number of samples
  *((uint32_t *)(cfg + 4)) = 1024 * 1024 - 1;

  // enter normal mode
  *((uint32_t *)(cfg + 0)) |= 2;

  // wait 1 second
  sleep(1);

  // print IN1 and IN2 samples
  for(i = 0; i < 1024 * 1024; ++i)
  {
    value[0] = *((int16_t *)(ram + 4*i + 0));
    value[1] = *((int16_t *)(ram + 4*i + 2));
    printf("%5d %5d\n", value[0], value[1]);
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));
  munmap(ram, sysconf(_SC_PAGESIZE));

  return 0;
}
