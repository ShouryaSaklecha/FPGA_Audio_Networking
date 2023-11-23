library ieee;
use ieee.math_real.all;
use ieee.std_logic_1164.all;

-- Package declartion
package aud_param is
    
    component i2s_master is
        generic (
            DATA_WIDTH : natural := 32;
            PCM_PRECISION : natural := 18
        );
        port (
            clk             : in  std_logic;
            clk_1            : in  std_logic;
                
            -- I2S interface to MEMs mic
            i2s_lrcl        : out std_logic;    -- left/riight clk (word sel): 0 = left, 1 = right
            i2s_dout        : in  std_logic;    -- serial data: payload, msb first
            i2s_bclk        : out std_logic;    -- Bit clock: freq = sample rate * bits per channel * number of channels
                                                -- (should run at 2-4MHz). Changes when the next bit is ready.
            -- FIFO interface to MEMs mic
            fifo_din        : out std_logic_vector(DATA_WIDTH - 1 downto 0);
            fifo_w_stb      : out std_logic;    -- Write strobe: 1 = ready to write, 0 = busy
            fifo_full       : in  std_logic     -- 1 = not full, 0 = full
        );
    end component;

    component fifo is
        generic (
            DATA_WIDTH : positive := 32;
            FIFO_DEPTH : positive := 5
        );
        port (
            clkw    : in  std_logic;
            clkr    : in  std_logic;
            rst     : in  std_logic;
            wr      : in  std_logic;
            rd      : in  std_logic;
            din     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
            empty   : out std_logic;
            full    : out std_logic;
            dout    : out std_logic_vector(DATA_WIDTH-1 downto 0)
        );
    end component;
   
    component audio_pipeline is
        generic(
            PCM_PRECISION : integer := 18;
            PCM_WIDTH : integer := 24;
            DATA_WIDTH : integer := 32;
            FIFO_DEPTH : integer := 5;
            TRANSFER_LEN : integer := 5
        );
        port(
            clk: in std_logic;
            clk_1            : in  std_logic;
            rst: in std_logic;
    
            --------------------------------------------------
            -- I2S
            --------------------------------------------------
            i2s_bclk        : out std_logic;
            i2s_lrcl        : out std_logic;
            i2s_dout        : in  std_logic;
    
            --------------------------------------------------
            -- AXI4-Stream
            --------------------------------------------------
            axis_tdata      : out std_logic_vector(DATA_WIDTH-1 downto 0);
            axis_tvalid     : out std_logic;
            axis_tready     : in  std_logic;
            axis_tlast      : out std_logic
        );
    end component;
    
    component ctrl_bus is
        generic (
            ------------------------------------------------
            -- Control bus parameters
            ------------------------------------------------
    
            ------------------------------------------------
            -- AXI Lite parameters
            ------------------------------------------------
            -- Width of S_AXI data bus
            C_S_AXI_DATA_WIDTH	: integer	:= 32;
            -- Width of S_AXI address bus
            C_S_AXI_ADDR_WIDTH	: integer	:= 5
        );
        port (
            ------------------------------------------------
            -- Control bus signals (PUT YOUR REGISTERS HERE)
            ------------------------------------------------
            cb_control_reg      : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
            cb_status_reg       : in  std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
            cb_gain_reg         : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
    
            ------------------------------------------------
            -- AXI Lite signals
            ------------------------------------------------
            -- Global Clock Signal
            S_AXI_ACLK	: in std_logic;
            -- Global Reset Signal. This Signal is Active LOW
            S_AXI_ARESETN	: in std_logic;
            -- Write address (issued by master, acceped by Slave)
            S_AXI_AWADDR	: in std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
            -- Write channel Protection type. This signal indicates the
                -- privilege and security level of the transaction, and whether
                -- the transaction is a data access or an instruction access.
            S_AXI_AWPROT	: in std_logic_vector(2 downto 0);
            -- Write address valid. This signal indicates that the master signaling
                -- valid write address and control information.
            S_AXI_AWVALID	: in std_logic;
            -- Write address ready. This signal indicates that the slave is ready
                -- to accept an address and associated control signals.
            S_AXI_AWREADY	: out std_logic;
            -- Write data (issued by master, acceped by Slave) 
            S_AXI_WDATA	: in std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
            -- Write strobes. This signal indicates which byte lanes hold
                -- valid data. There is one write strobe bit for each eight
                -- bits of the write data bus.    
            S_AXI_WSTRB	: in std_logic_vector((C_S_AXI_DATA_WIDTH/8)-1 downto 0);
            -- Write valid. This signal indicates that valid write
                -- data and strobes are available.
            S_AXI_WVALID	: in std_logic;
            -- Write ready. This signal indicates that the slave
                -- can accept the write data.
            S_AXI_WREADY	: out std_logic;
            -- Write response. This signal indicates the status
                -- of the write transaction.
            S_AXI_BRESP	: out std_logic_vector(1 downto 0);
            -- Write response valid. This signal indicates that the channel
                -- is signaling a valid write response.
            S_AXI_BVALID	: out std_logic;
            -- Response ready. This signal indicates that the master
                -- can accept a write response.
            S_AXI_BREADY	: in std_logic;
            -- Read address (issued by master, acceped by Slave)
            S_AXI_ARADDR	: in std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
            -- Protection type. This signal indicates the privilege
                -- and security level of the transaction, and whether the
                -- transaction is a data access or an instruction access.
            S_AXI_ARPROT	: in std_logic_vector(2 downto 0);
            -- Read address valid. This signal indicates that the channel
                -- is signaling valid read address and control information.
            S_AXI_ARVALID	: in std_logic;
            -- Read address ready. This signal indicates that the slave is
                -- ready to accept an address and associated control signals.
            S_AXI_ARREADY	: out std_logic;
            -- Read data (issued by slave)
            S_AXI_RDATA	: out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
            -- Read response. This signal indicates the status of the
                -- read transfer.
            S_AXI_RRESP	: out std_logic_vector(1 downto 0);
            -- Read valid. This signal indicates that the channel is
                -- signaling the required read data.
            S_AXI_RVALID	: out std_logic;
            -- Read ready. This signal indicates that the master can
                -- accept the read data and response information.
            S_AXI_RREADY	: in std_logic
        );
    end component;


end package aud_param;

-- Body section
package body aud_param is

end package body aud_param;