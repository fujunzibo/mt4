using System;
using System.IO;
using System.Windows.Forms;
using System.Data;
using System.Globalization;
using System.Linq;

namespace HedgeArbController
{
    public partial class MainForm : Form
    {
        private System.Windows.Forms.Timer refreshTimer;
        private string commonFilesPath;
        private string symbol;
        
        // 保存滚动位置
        private int dgvPositionsAScrollPosition = 0;
        private int dgvPositionsBScrollPosition = 0;
        private int dgvHistoryAScrollPosition = 0;
        private int dgvHistoryBScrollPosition = 0;

        public MainForm()
        {
            InitializeComponent();
            
            // 默认路径
            string defaultPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                "MetaQuotes", "Terminal", "Common", "Files");
            txtCommonPath.Text = defaultPath;
            commonFilesPath = defaultPath;
            
            txtSymbol.Text = "GOLD";
            symbol = "GOLD";
            
            // 定时刷新
            refreshTimer = new System.Windows.Forms.Timer();
            refreshTimer.Interval = 1000; // 1秒
            refreshTimer.Tick += RefreshTimer_Tick;
            refreshTimer.Start();
            
            // 立即刷新一次
            RefreshData();
        }

        private void RefreshTimer_Tick(object? sender, EventArgs e)
        {
            RefreshData();
        }

        private void RefreshData()
        {
            try
            {
                // 读取报价
                ReadPriceData();
                
                // 读取盈亏
                ReadProfitData();
                
                // 读取账户信息
                ReadAccountData();
                
                // 读取持仓信息
                ReadPositionsData();
                
                // 读取交易历史
                ReadTradeHistory();
                
                // 读取最近指令
                ReadLastCommand();
            }
            catch
            {
                // 静默处理错误，避免界面卡顿
            }
        }

        private void ReadPriceData()
        {
            // 确保 symbol 不为空
            if (string.IsNullOrWhiteSpace(symbol))
            {
                symbol = "GOLD"; // 默认值
                if (txtSymbol != null)
                {
                    txtSymbol.Text = symbol;
                }
            }
            
            string fileA = Path.Combine(commonFilesPath, $"ArbPrice_{symbol}_A.csv");
            string fileB = Path.Combine(commonFilesPath, $"ArbPrice_{symbol}_B.csv");
            
            double priceA = 0, priceB = 0;
            bool haveA = false, haveB = false;
            
            // 读 A
            if (File.Exists(fileA))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileA);
                    if (lines.Length > 0)
                    {
                        string[] parts = lines[0].Split(';');
                        if (parts.Length >= 3)
                        {
                            double bid = double.Parse(parts[1]);
                            double ask = double.Parse(parts[2]);
                            priceA = (bid + ask) / 2.0;
                            haveA = true;
                        }
                    }
                }
                catch { }
            }
            
            // 读 B
            if (File.Exists(fileB))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileB);
                    if (lines.Length > 0)
                    {
                        string[] parts = lines[0].Split(';');
                        if (parts.Length >= 3)
                        {
                            double bid = double.Parse(parts[1]);
                            double ask = double.Parse(parts[2]);
                            priceB = (bid + ask) / 2.0;
                            haveB = true;
                        }
                    }
                }
                catch { }
            }
            
            // 更新界面
            lblPriceA.Text = haveA ? priceA.ToString("F5") : "-";
            lblPriceB.Text = haveB ? priceB.ToString("F5") : "-";
            
            if (haveA && haveB)
            {
                double diff = priceA - priceB;
                lblPriceDiff.Text = diff.ToString("F5");
            }
            else
            {
                lblPriceDiff.Text = "-";
            }
        }

        private void ReadProfitData()
        {
            // 读取美元盈亏文件（profitUSD.csv）
            string fileA = Path.Combine(commonFilesPath, $"ArbState_{symbol}_A_profitUSD.csv");
            string fileB = Path.Combine(commonFilesPath, $"ArbState_{symbol}_B_profitUSD.csv");
            
            double profitA = 0, profitB = 0;
            bool haveA = false, haveB = false;
            
            // 读 A
            if (File.Exists(fileA))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileA);
                    if (lines.Length > 0)
                    {
                        string[] parts = lines[0].Split(';');
                        if (parts.Length >= 2)
                        {
                            profitA = double.Parse(parts[1]);
                            haveA = true;
                        }
                    }
                }
                catch { }
            }
            
            // 读 B
            if (File.Exists(fileB))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileB);
                    if (lines.Length > 0)
                    {
                        string[] parts = lines[0].Split(';');
                        if (parts.Length >= 2)
                        {
                            profitB = double.Parse(parts[1]);
                            haveB = true;
                        }
                    }
                }
                catch { }
            }
            
            // 更新界面（显示美元盈亏）
            lblProfitA.Text = haveA ? profitA.ToString("F2") : "-";
            lblProfitB.Text = haveB ? profitB.ToString("F2") : "-";
            
            if (haveA && haveB)
            {
                double total = profitA + profitB;
                lblTotalProfit.Text = total.ToString("F2");
            }
            else
            {
                lblTotalProfit.Text = "-";
            }
        }

        private void ReadAccountData()
        {
            string fileA = Path.Combine(commonFilesPath, $"ArbState_Account_{symbol}_A.csv");
            string fileB = Path.Combine(commonFilesPath, $"ArbState_Account_{symbol}_B.csv");
            
            // 读 A
            if (File.Exists(fileA))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileA);
                    if (lines.Length > 0)
                    {
                        string[] parts = lines[0].Split(';');
                        // 至少需要3列（时间、BrokerId、账号）
                        if (parts.Length >= 3)
                        {
                            // 读取基本账户信息
                            if (parts.Length > 2) lblAccountA.Text = parts[2]; // 账号
                            if (parts.Length > 3) lblBalanceA.Text = parts[3]; // 余额
                            if (parts.Length > 4) lblEquityA.Text = parts[4];  // 净值
                            if (parts.Length > 5) lblMarginA.Text = parts[5];  // 已用保证金
                            if (parts.Length > 6) lblFreeMarginA.Text = parts[6]; // 可用保证金
                            if (parts.Length > 7) lblLeverageA.Text = parts[7]; // 杠杆
                            
                            // 读取Bid和Ask（即使列数不够，也尝试读取）
                            if (parts.Length > 8)
                            {
                                string bidValue = parts[8].Trim();
                                lblBidA.Text = string.IsNullOrEmpty(bidValue) ? "-" : bidValue;
                            }
                            else
                            {
                                lblBidA.Text = "-";
                            }
                            
                            if (parts.Length > 9)
                            {
                                string askValue = parts[9].Trim();
                                lblAskA.Text = string.IsNullOrEmpty(askValue) ? "-" : askValue;
                            }
                            else
                            {
                                lblAskA.Text = "-";
                            }
                        }
                        else
                        {
                            // 文件格式不正确，显示"-"
                            lblAccountA.Text = "-";
                            lblBalanceA.Text = "-";
                            lblEquityA.Text = "-";
                            lblMarginA.Text = "-";
                            lblFreeMarginA.Text = "-";
                            lblLeverageA.Text = "-";
                            lblBidA.Text = "-";
                            lblAskA.Text = "-";
                        }
                    }
                    else
                    {
                        // 文件为空
                        lblAccountA.Text = "-";
                        lblBalanceA.Text = "-";
                        lblEquityA.Text = "-";
                        lblMarginA.Text = "-";
                        lblFreeMarginA.Text = "-";
                        lblLeverageA.Text = "-";
                        lblBidA.Text = "-";
                        lblAskA.Text = "-";
                    }
                }
                catch (Exception ex)
                {
                    // 读取错误，显示"-"
                    System.Diagnostics.Debug.WriteLine($"读取账户A信息错误: {ex.Message}");
                    lblAccountA.Text = "-";
                    lblBalanceA.Text = "-";
                    lblEquityA.Text = "-";
                    lblMarginA.Text = "-";
                    lblFreeMarginA.Text = "-";
                    lblLeverageA.Text = "-";
                    lblBidA.Text = "-";
                    lblAskA.Text = "-";
                }
            }
            else
            {
                lblAccountA.Text = "-";
                lblBalanceA.Text = "-";
                lblEquityA.Text = "-";
                lblMarginA.Text = "-";
                lblFreeMarginA.Text = "-";
                lblLeverageA.Text = "-";
                lblBidA.Text = "-";
                lblAskA.Text = "-";
            }
            
            // 读 B
            if (File.Exists(fileB))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileB);
                    if (lines.Length > 0)
                    {
                        string[] parts = lines[0].Split(';');
                        // 至少需要3列（时间、BrokerId、账号）
                        if (parts.Length >= 3)
                        {
                            // 读取基本账户信息
                            if (parts.Length > 2) lblAccountB.Text = parts[2];
                            if (parts.Length > 3) lblBalanceB.Text = parts[3];
                            if (parts.Length > 4) lblEquityB.Text = parts[4];
                            if (parts.Length > 5) lblMarginB.Text = parts[5];
                            if (parts.Length > 6) lblFreeMarginB.Text = parts[6];
                            if (parts.Length > 7) lblLeverageB.Text = parts[7];
                            
                            // 读取Bid和Ask（即使列数不够，也尝试读取）
                            if (parts.Length > 8)
                            {
                                string bidValue = parts[8].Trim();
                                lblBidB.Text = string.IsNullOrEmpty(bidValue) ? "-" : bidValue;
                            }
                            else
                            {
                                lblBidB.Text = "-";
                            }
                            
                            if (parts.Length > 9)
                            {
                                string askValue = parts[9].Trim();
                                lblAskB.Text = string.IsNullOrEmpty(askValue) ? "-" : askValue;
                            }
                            else
                            {
                                lblAskB.Text = "-";
                            }
                        }
                        else
                        {
                            // 文件格式不正确，显示"-"
                            lblAccountB.Text = "-";
                            lblBalanceB.Text = "-";
                            lblEquityB.Text = "-";
                            lblMarginB.Text = "-";
                            lblFreeMarginB.Text = "-";
                            lblLeverageB.Text = "-";
                            lblBidB.Text = "-";
                            lblAskB.Text = "-";
                        }
                    }
                    else
                    {
                        // 文件为空
                        lblAccountB.Text = "-";
                        lblBalanceB.Text = "-";
                        lblEquityB.Text = "-";
                        lblMarginB.Text = "-";
                        lblFreeMarginB.Text = "-";
                        lblLeverageB.Text = "-";
                        lblBidB.Text = "-";
                        lblAskB.Text = "-";
                    }
                }
                catch (Exception ex)
                {
                    // 读取错误，显示"-"
                    System.Diagnostics.Debug.WriteLine($"读取账户B信息错误: {ex.Message}");
                    lblAccountB.Text = "-";
                    lblBalanceB.Text = "-";
                    lblEquityB.Text = "-";
                    lblMarginB.Text = "-";
                    lblFreeMarginB.Text = "-";
                    lblLeverageB.Text = "-";
                    lblBidB.Text = "-";
                    lblAskB.Text = "-";
                }
            }
            else
            {
                lblAccountB.Text = "-";
                lblBalanceB.Text = "-";
                lblEquityB.Text = "-";
                lblMarginB.Text = "-";
                lblFreeMarginB.Text = "-";
                lblLeverageB.Text = "-";
                lblBidB.Text = "-";
                lblAskB.Text = "-";
            }
        }

        private void ReadLastCommand()
        {
            string file = Path.Combine(commonFilesPath, $"ArbState_{symbol}.txt");
            if (File.Exists(file))
            {
                try
                {
                    string content = File.ReadAllText(file).Trim();
                    lblLastCommand.Text = content;
                }
                catch
                {
                    lblLastCommand.Text = "-";
                }
            }
            else
            {
                lblLastCommand.Text = "-";
            }
        }

        // 已移除手动平仓功能，改为完全自动平仓

        private void txtCommonPath_TextChanged(object sender, EventArgs e)
        {
            commonFilesPath = txtCommonPath.Text;
        }

        private void txtSymbol_TextChanged(object sender, EventArgs e)
        {
            if (txtSymbol != null && !string.IsNullOrWhiteSpace(txtSymbol.Text))
            {
                symbol = txtSymbol.Text.Trim().ToUpper();
            }
            else
            {
                symbol = "GOLD"; // 默认值
            }
        }

        private void ReadPositionsData()
        {
            // 读取 Broker A 的持仓
            string fileA = Path.Combine(commonFilesPath, $"ArbState_Positions_{symbol}_A.csv");
            
            // 调试：检查文件是否存在
            bool fileAExists = File.Exists(fileA);
            
            DataTable dtA = new DataTable();
            dtA.Columns.Add("订单号", typeof(string));
            dtA.Columns.Add("类型", typeof(string));
            dtA.Columns.Add("手数", typeof(string));
            dtA.Columns.Add("开仓价", typeof(string));
            dtA.Columns.Add("当前价", typeof(string));
            dtA.Columns.Add("盈亏", typeof(string));
            dtA.Columns.Add("库存费", typeof(string));
            dtA.Columns.Add("佣金", typeof(string));
            dtA.Columns.Add("开仓时间", typeof(string));

            if (fileAExists)
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileA);
                    // 调试：记录文件行数
                    int dataRowCount = 0;
                    for (int i = 1; i < lines.Length; i++) // 跳过表头
                    {
                        if (string.IsNullOrWhiteSpace(lines[i])) continue;
                        string[] parts = lines[i].Split(';');
                        // 检查列数，如果不够9列，尝试用其他分隔符或跳过
                        if (parts.Length >= 9)
                        {
                            // 转换类型：BUY -> 开多，SELL -> 开空
                            string typeStr = parts[1].Trim().ToUpper();
                            string typeDisplay = (typeStr == "BUY") ? "开多" : (typeStr == "SELL") ? "开空" : parts[1];
                            
                            // ==== 修复：转换时间格式，将UTC时间转换为本地时间 ====
                            string timeStr = ConvertMT4TimeToLocal(parts[8]);
                            
                            dtA.Rows.Add(parts[0], typeDisplay, parts[2], parts[3], parts[4], 
                                       parts[5], parts[6], parts[7], timeStr);
                            dataRowCount++;
                        }
                        // 如果列数不够，可能是文件格式问题，跳过这一行
                    }
                }
                catch
                {
                    // 读取文件出错，静默处理
                    // 如果需要调试，可以在这里添加日志输出
                }
            }

            // 读取 Broker B 的持仓
            string fileB = Path.Combine(commonFilesPath, $"ArbState_Positions_{symbol}_B.csv");
            bool fileBExists = File.Exists(fileB);
            
            DataTable dtB = new DataTable();
            dtB.Columns.Add("订单号", typeof(string));
            dtB.Columns.Add("类型", typeof(string));
            dtB.Columns.Add("手数", typeof(string));
            dtB.Columns.Add("开仓价", typeof(string));
            dtB.Columns.Add("当前价", typeof(string));
            dtB.Columns.Add("盈亏", typeof(string));
            dtB.Columns.Add("库存费", typeof(string));
            dtB.Columns.Add("佣金", typeof(string));
            dtB.Columns.Add("开仓时间", typeof(string));

            if (File.Exists(fileB))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileB);
                    for (int i = 1; i < lines.Length; i++)
                    {
                        if (string.IsNullOrWhiteSpace(lines[i])) continue;
                        string[] parts = lines[i].Split(';');
                        if (parts.Length >= 9)
                        {
                            // 转换类型：BUY -> 开多，SELL -> 开空
                            string typeStr = parts[1].Trim().ToUpper();
                            string typeDisplay = (typeStr == "BUY") ? "开多" : (typeStr == "SELL") ? "开空" : parts[1];
                            
                            // ==== 修复：转换时间格式，将UTC时间转换为本地时间 ====
                            string timeStr = ConvertMT4TimeToLocal(parts[8]);
                            
                            dtB.Rows.Add(parts[0], typeDisplay, parts[2], parts[3], parts[4], 
                                       parts[5], parts[6], parts[7], timeStr);
                        }
                    }
                }
                catch { }
            }

            // 更新 DataGridView（已在 UI 线程中，无需 Invoke）
            if (this.IsHandleCreated && dgvPositionsA != null && dgvPositionsB != null)
            {
                try
                {
                    // 保存滚动位置
                    if (dgvPositionsA.FirstDisplayedScrollingRowIndex >= 0)
                        dgvPositionsAScrollPosition = dgvPositionsA.FirstDisplayedScrollingRowIndex;
                    if (dgvPositionsB.FirstDisplayedScrollingRowIndex >= 0)
                        dgvPositionsBScrollPosition = dgvPositionsB.FirstDisplayedScrollingRowIndex;
                    
                    // 清除现有数据源
                    dgvPositionsA.DataSource = null;
                    dgvPositionsB.DataSource = null;
                    
                    // 设置新数据源
                    dgvPositionsA.DataSource = dtA;
                    dgvPositionsB.DataSource = dtB;
                    
                    // 确保自动生成列，并设置列宽
                    if (dgvPositionsA.Columns.Count > 0)
                    {
                        dgvPositionsA.AutoGenerateColumns = true;
                        // ==== 修复：使用Fill模式，避免列宽自动调整导致滚动条问题 ====
                        dgvPositionsA.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvPositionsA.Columns)
                        {
                            col.Visible = true;
                        }
                        // 恢复滚动位置
                        if (dgvPositionsAScrollPosition >= 0 && dgvPositionsAScrollPosition < dgvPositionsA.Rows.Count)
                        {
                            dgvPositionsA.FirstDisplayedScrollingRowIndex = dgvPositionsAScrollPosition;
                        }
                    }
                    if (dgvPositionsB.Columns.Count > 0)
                    {
                        dgvPositionsB.AutoGenerateColumns = true;
                        // ==== 修复：使用Fill模式，避免列宽自动调整导致滚动条问题 ====
                        dgvPositionsB.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvPositionsB.Columns)
                        {
                            col.Visible = true;
                        }
                        // 恢复滚动位置
                        if (dgvPositionsBScrollPosition >= 0 && dgvPositionsBScrollPosition < dgvPositionsB.Rows.Count)
                        {
                            dgvPositionsB.FirstDisplayedScrollingRowIndex = dgvPositionsBScrollPosition;
                        }
                    }
                    
                    // 设置盈亏列的颜色和格式（第6列，索引5）
                    if (dgvPositionsA.Columns.Count > 5)
                    {
                        dgvPositionsA.Columns[5].DefaultCellStyle.Format = "F2";
                        dgvPositionsA.Columns[5].DefaultCellStyle.ForeColor = System.Drawing.Color.Black;
                    }
                    if (dgvPositionsB.Columns.Count > 5)
                    {
                        dgvPositionsB.Columns[5].DefaultCellStyle.Format = "F2";
                        dgvPositionsB.Columns[5].DefaultCellStyle.ForeColor = System.Drawing.Color.Black;
                    }
                    
                    // ==== 修复：启用垂直滚动条，确保可以滚动 ====
                    dgvPositionsA.ScrollBars = ScrollBars.Vertical;
                    dgvPositionsB.ScrollBars = ScrollBars.Vertical;
                    
                    // 刷新显示
                    dgvPositionsA.Refresh();
                    dgvPositionsB.Refresh();
                }
                catch (Exception ex)
                {
                    // 调试：输出错误信息
                    System.Diagnostics.Debug.WriteLine($"ReadPositionsData 错误: {ex.Message}");
                }
            }
        }

        private void ReadTradeHistory()
        {
            // 读取 Broker A 的交易历史
            string fileA = Path.Combine(commonFilesPath, $"ArbState_History_{symbol}_A.csv");
            
            DataTable dtA = new DataTable();
            dtA.Columns.Add("时间", typeof(string));
            dtA.Columns.Add("动作", typeof(string));
            dtA.Columns.Add("订单号", typeof(string));
            dtA.Columns.Add("类型", typeof(string));
            dtA.Columns.Add("手数", typeof(string));
            dtA.Columns.Add("开仓价", typeof(string));
            dtA.Columns.Add("平仓价", typeof(string));
            dtA.Columns.Add("盈亏", typeof(string));
            // ==== 修复：去掉备注列 ====

            if (File.Exists(fileA))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileA);
                    for (int i = 1; i < lines.Length; i++) // 跳过表头
                    {
                        if (string.IsNullOrWhiteSpace(lines[i])) continue;
                        string[] parts = lines[i].Split(';');
                        // 新格式：Time;Action;Ticket;Type;Lots;OpenPrice;ClosePrice;Profit;Comment (9列)
                        // 旧格式兼容：Time;Action;Ticket;Type;Lots;Price;Profit;Comment (8列)
                        if (parts.Length >= 9)
                        {
                            // 新格式：有开仓价和平仓价
                            // 转换类型：BUY -> 开多，SELL -> 开空
                            string typeStr = parts[3].Trim().ToUpper();
                            string typeDisplay = (typeStr == "BUY") ? "开多" : (typeStr == "SELL") ? "开空" : parts[3];
                            
                            // ==== 修复：转换时间格式，将UTC时间转换为本地时间 ====
                            string timeStr = ConvertMT4TimeToLocal(parts[0]);
                            
                            // ==== 修复：去掉备注列（parts[8]） ====
                            dtA.Rows.Add(timeStr, parts[1], parts[2], typeDisplay, parts[4], 
                                       parts[5], parts[6], parts[7]);
                        }
                        else if (parts.Length >= 8)
                        {
                            // 旧格式兼容：只有平仓价，开仓价显示为"-"
                            // 转换类型：BUY -> 开多，SELL -> 开空
                            string typeStr = parts[3].Trim().ToUpper();
                            string typeDisplay = (typeStr == "BUY") ? "开多" : (typeStr == "SELL") ? "开空" : parts[3];
                            
                            // ==== 修复：转换时间格式，将UTC时间转换为本地时间 ====
                            string timeStr = ConvertMT4TimeToLocal(parts[0]);
                            
                            // ==== 修复：去掉备注列 ====
                            dtA.Rows.Add(timeStr, parts[1], parts[2], typeDisplay, parts[4], 
                                       "-", parts[5], parts[6]);
                        }
                    }
                }
                catch { }
            }

            // 读取 Broker B 的交易历史
            string fileB = Path.Combine(commonFilesPath, $"ArbState_History_{symbol}_B.csv");
            DataTable dtB = new DataTable();
            dtB.Columns.Add("时间", typeof(string));
            dtB.Columns.Add("动作", typeof(string));
            dtB.Columns.Add("订单号", typeof(string));
            dtB.Columns.Add("类型", typeof(string));
            dtB.Columns.Add("手数", typeof(string));
            dtB.Columns.Add("开仓价", typeof(string));
            dtB.Columns.Add("平仓价", typeof(string));
            dtB.Columns.Add("盈亏", typeof(string));
            // ==== 修复：去掉备注列 ====

            if (File.Exists(fileB))
            {
                try
                {
                    string[] lines = File.ReadAllLines(fileB);
                    for (int i = 1; i < lines.Length; i++)
                    {
                        if (string.IsNullOrWhiteSpace(lines[i])) continue;
                        string[] parts = lines[i].Split(';');
                        // 新格式：Time;Action;Ticket;Type;Lots;OpenPrice;ClosePrice;Profit;Comment (9列)
                        // 旧格式兼容：Time;Action;Ticket;Type;Lots;Price;Profit;Comment (8列)
                        if (parts.Length >= 9)
                        {
                            // 新格式：有开仓价和平仓价
                            // 转换类型：BUY -> 开多，SELL -> 开空
                            string typeStr = parts[3].Trim().ToUpper();
                            string typeDisplay = (typeStr == "BUY") ? "开多" : (typeStr == "SELL") ? "开空" : parts[3];
                            
                            // ==== 修复：转换时间格式，将UTC时间转换为本地时间 ====
                            string timeStr = ConvertMT4TimeToLocal(parts[0]);
                            
                            // ==== 修复：去掉备注列（parts[8]） ====
                            dtB.Rows.Add(timeStr, parts[1], parts[2], typeDisplay, parts[4], 
                                       parts[5], parts[6], parts[7]);
                        }
                        else if (parts.Length >= 8)
                        {
                            // 旧格式兼容：只有平仓价，开仓价显示为"-"
                            // 转换类型：BUY -> 开多，SELL -> 开空
                            string typeStr = parts[3].Trim().ToUpper();
                            string typeDisplay = (typeStr == "BUY") ? "开多" : (typeStr == "SELL") ? "开空" : parts[3];
                            
                            // ==== 修复：转换时间格式，将UTC时间转换为本地时间 ====
                            string timeStr = ConvertMT4TimeToLocal(parts[0]);
                            
                            // ==== 修复：去掉备注列 ====
                            dtB.Rows.Add(timeStr, parts[1], parts[2], typeDisplay, parts[4], 
                                       "-", parts[5], parts[6]);
                        }
                    }
                }
                catch { }
            }

            // 更新 DataGridView（已在 UI 线程中，无需 Invoke）
            if (this.IsHandleCreated && dgvHistoryA != null && dgvHistoryB != null)
            {
                try
                {
                    // 保存滚动位置
                    if (dgvHistoryA.FirstDisplayedScrollingRowIndex >= 0)
                        dgvHistoryAScrollPosition = dgvHistoryA.FirstDisplayedScrollingRowIndex;
                    if (dgvHistoryB.FirstDisplayedScrollingRowIndex >= 0)
                        dgvHistoryBScrollPosition = dgvHistoryB.FirstDisplayedScrollingRowIndex;
                    
                    // 清除现有数据源
                    dgvHistoryA.DataSource = null;
                    dgvHistoryB.DataSource = null;
                    
                    // 设置新数据源
                    dgvHistoryA.DataSource = dtA;
                    dgvHistoryB.DataSource = dtB;
                    
                    // 确保自动生成列，并设置列宽
                    if (dgvHistoryA.Columns.Count > 0)
                    {
                        dgvHistoryA.AutoGenerateColumns = true;
                        // ==== 修复：使用Fill模式，避免列宽自动调整导致滚动条问题 ====
                        dgvHistoryA.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvHistoryA.Columns)
                        {
                            col.Visible = true;
                        }
                        // 恢复滚动位置
                        if (dgvHistoryAScrollPosition >= 0 && dgvHistoryAScrollPosition < dgvHistoryA.Rows.Count)
                        {
                            dgvHistoryA.FirstDisplayedScrollingRowIndex = dgvHistoryAScrollPosition;
                        }
                    }
                    if (dgvHistoryB.Columns.Count > 0)
                    {
                        dgvHistoryB.AutoGenerateColumns = true;
                        // ==== 修复：使用Fill模式，避免列宽自动调整导致滚动条问题 ====
                        dgvHistoryB.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvHistoryB.Columns)
                        {
                            col.Visible = true;
                        }
                        // 恢复滚动位置
                        if (dgvHistoryBScrollPosition >= 0 && dgvHistoryBScrollPosition < dgvHistoryB.Rows.Count)
                        {
                            dgvHistoryB.FirstDisplayedScrollingRowIndex = dgvHistoryBScrollPosition;
                        }
                    }
                    
                    // 设置盈亏列的颜色和格式（第8列，索引7，去掉备注后变成第7列，索引6）
                    if (dgvHistoryA.Columns.Count > 6)
                    {
                        dgvHistoryA.Columns[6].DefaultCellStyle.Format = "F2";
                        dgvHistoryA.Columns[6].DefaultCellStyle.ForeColor = System.Drawing.Color.Black;
                    }
                    if (dgvHistoryB.Columns.Count > 6)
                    {
                        dgvHistoryB.Columns[6].DefaultCellStyle.Format = "F2";
                        dgvHistoryB.Columns[6].DefaultCellStyle.ForeColor = System.Drawing.Color.Black;
                    }
                    
                    // ==== 修复：隐藏备注列（如果存在） ====
                    if (dgvHistoryA.Columns.Count > 7 && dgvHistoryA.Columns.Contains("备注"))
                    {
                        dgvHistoryA.Columns["备注"].Visible = false;
                    }
                    if (dgvHistoryB.Columns.Count > 7 && dgvHistoryB.Columns.Contains("备注"))
                    {
                        dgvHistoryB.Columns["备注"].Visible = false;
                    }
                    
                    // ==== 修复：启用垂直滚动条，确保可以滚动 ====
                    dgvHistoryA.ScrollBars = ScrollBars.Vertical;
                    dgvHistoryB.ScrollBars = ScrollBars.Vertical;
                    
                    // 刷新显示
                    dgvHistoryA.Refresh();
                    dgvHistoryB.Refresh();
                }
                catch (Exception ex)
                {
                    // 调试：输出错误信息
                    System.Diagnostics.Debug.WriteLine($"ReadTradeHistory 错误: {ex.Message}");
                }
            }
        }
        
        // ==== 修复：将MT4的时间转换为本地时间 ====
        // MT4的TimeToStr格式通常是 "2025.12.11 20:31:19" 或 "2025.12.11 20:31:19.473"
        // MT4返回的是服务器时间（通常是UTC），需要转换为本地时间
        // 如果时间比客户端早一个小时，需要加一个小时来修正
        private string ConvertMT4TimeToLocal(string mt4TimeStr)
        {
            if (string.IsNullOrWhiteSpace(mt4TimeStr))
                return mt4TimeStr;
            
            try
            {
                // 尝试解析MT4时间格式：2025.12.11 20:31:19 或 2025.12.11 20:31:19.473
                // 先处理毫秒部分（如果有）
                string timeStr = mt4TimeStr.Trim();
                if (timeStr.Contains("."))
                {
                    int lastDotIndex = timeStr.LastIndexOf('.');
                    if (lastDotIndex > 0)
                    {
                        // 检查是否是毫秒部分（小数点后3位数字）
                        string afterLastDot = timeStr.Substring(lastDotIndex + 1);
                        if (afterLastDot.Length == 3 && afterLastDot.All(char.IsDigit))
                        {
                            // 这是毫秒，移除它（我们不需要毫秒）
                            timeStr = timeStr.Substring(0, lastDotIndex);
                        }
                    }
                }
                
                // 解析时间字符串：格式 "2025.12.11 20:31:19"
                // 将点号替换为斜杠，以便DateTime.Parse识别
                string dateTimeStr = timeStr.Replace(".", "/");
                
                // ==== 修复：假设MT4返回的是UTC时间，转换为本地时间 ====
                // 使用SpecifyKind明确指定为UTC时间
                DateTime utcTime;
                if (DateTime.TryParseExact(dateTimeStr, "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture, 
                    DateTimeStyles.None, out utcTime))
                {
                    // 明确指定为UTC时间
                    utcTime = DateTime.SpecifyKind(utcTime, DateTimeKind.Utc);
                    // 转换为本地时间
                    DateTime localTime = utcTime.ToLocalTime();
                    
                    // ==== 修复：如果时间比客户端早一个小时，加一个小时来修正 ====
                    // 这可能是因为MT4服务器时区设置的问题
                    localTime = localTime.AddHours(1);
                    
                    // ==== 修复：返回完整时间格式（包括秒） ====
                    return localTime.ToString("yyyy.MM.dd HH:mm:ss");
                }
                else if (DateTime.TryParse(dateTimeStr, CultureInfo.InvariantCulture, 
                    DateTimeStyles.None, out utcTime))
                {
                    // 如果标准格式失败，尝试通用解析
                    utcTime = DateTime.SpecifyKind(utcTime, DateTimeKind.Utc);
                    DateTime localTime = utcTime.ToLocalTime();
                    
                    // ==== 修复：如果时间比客户端早一个小时，加一个小时来修正 ====
                    localTime = localTime.AddHours(1);
                    
                    // ==== 修复：返回完整时间格式（包括秒） ====
                    return localTime.ToString("yyyy.MM.dd HH:mm:ss");
                }
                else
                {
                    // 解析失败，返回原始字符串（保持完整格式）
                    return mt4TimeStr;
                }
            }
            catch
            {
                // 解析失败，返回原始字符串
                return mt4TimeStr;
            }
        }
    }
}

