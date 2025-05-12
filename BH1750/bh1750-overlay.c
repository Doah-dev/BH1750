/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2835";

    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;

            bh1750@23 {
                compatible = "rohm,BH1750";
                reg = <0x23>;   // Or 0x5C depending on ADDR pin
            };
        };
    };
};
