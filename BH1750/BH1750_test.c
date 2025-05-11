#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>

#define IOCTL_SET_POWER_ON _IOW('k', 1, int)
#define IOCTL_SET_POWER_OFF _IOW('k', 2, int)
#define IOCTL_READ_Continuously_H_Mode1 _IOR('k', 3, int)
#define IOCTL_READ_One_H_Mode1 _IOR('k', 4, int)
#define IOCTL_READ_Continuously_H_Mode2 _IOR('k', 5, int)
#define IOCTL_READ_One_H_Mode2 _IOR('k', 6, int)
#define IOCTL_RESET _IOW('k', 7, int)
#define IOCTL_CHANGE_Measurement_Time _IOW('k', 8, int)
#define IOCTL_READ_Continuously_L_Mode _IOR('k', 9, int)
#define IOCTL_READ_One_L_Mode _IOR('k', 10, int)

#define DEVICE_PATH "/dev/BH1750"

int main()
{
    int fd;
    int data;

    // Mở thiết bị
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return errno;
    }

    // Bật nguồn thiết bị
    data = 1; // Power On
    if (ioctl(fd, IOCTL_SET_POWER_ON, &data) < 0) {
        perror("Failed to power on the device");
        close(fd);
        return errno;
    }

    // Thay đổi thời gian đo
    data = 44;  // Measurement time setting
    if (ioctl(fd, IOCTL_CHANGE_Measurement_Time, &data) < 0) {
        perror("Failed to change measurement time");
        close(fd);
        return errno;
    }

    // Đọc liên tục từ chế độ H mode1
    while (1) {
        if (ioctl(fd, IOCTL_READ_One_H_Mode1, &data) < 0) {
            perror("Failed to read continuously from Mode 1");
            close(fd);
            return errno;
        }
        printf("%d [lux]\n", data);

        // Ngủ 1 giây
        sleep(1);
    }

    // Đóng thiết bị
    close(fd);
    return 0;
}
