using System;
using System.IO;
using System.Windows.Forms;
using System.Data;

namespace HedgeArbController
{
    public partial class MainForm : Form
    {
        private System.Windows.Forms.Timer refreshTimer;
        private string commonFilesPath;
        private string symbol;

        public MainForm()
        {
            InitializeComponent();
            
            // 默认路径
            string defaultPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                "MetaQuotes", "Terminal", "Common", "Files");
            txtCommonPath.Text = defaultPath;
            commonFilesPath = defaultPath;
            
            txtSymbol.Text = "EURUSD";
            symbol = "EURUSD";
            
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
                symbol = "EURUSD"; // 默认值
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
            string fileA = Path.Combine(commonFilesPath, $"ArbState_{symbol}_A_profit.csv");
            string fileB = Path.Combine(commonFilesPath, $"ArbState_{symbol}_B_profit.csv");
            
            double pipsA = 0, pipsB = 0;
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
                            pipsA = double.Parse(parts[1]);
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
                            pipsB = double.Parse(parts[1]);
                            haveB = true;
                        }
                    }
                }
                catch { }
            }
            
            // 更新界面
            lblProfitA.Text = haveA ? pipsA.ToString("F2") : "-";
            lblProfitB.Text = haveB ? pipsB.ToString("F2") : "-";
            
            if (haveA && haveB)
            {
                double total = pipsA + pipsB;
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
                        if (parts.Length >= 10)
                        {
                            lblAccountA.Text = parts[2]; // 账号
                            lblBalanceA.Text = parts[3]; // 余额
                            lblEquityA.Text = parts[4];  // 净值
                            lblMarginA.Text = parts[5];  // 已用保证金
                            lblFreeMarginA.Text = parts[6]; // 可用保证金
                            lblLeverageA.Text = parts[7]; // 杠杆
                            lblBidA.Text = parts[8];     // Bid
                            lblAskA.Text = parts[9];     // Ask
                        }
                    }
                }
                catch { }
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
                        if (parts.Length >= 10)
                        {
                            lblAccountB.Text = parts[2];
                            lblBalanceB.Text = parts[3];
                            lblEquityB.Text = parts[4];
                            lblMarginB.Text = parts[5];
                            lblFreeMarginB.Text = parts[6];
                            lblLeverageB.Text = parts[7];
                            lblBidB.Text = parts[8];
                            lblAskB.Text = parts[9];
                        }
                    }
                }
                catch { }
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
                symbol = "EURUSD"; // 默认值
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
                            dtA.Rows.Add(parts[0], parts[1], parts[2], parts[3], parts[4], 
                                       parts[5], parts[6], parts[7], parts[8]);
                            dataRowCount++;
                        }
                        // 如果列数不够，可能是文件格式问题，跳过这一行
                    }
                }
                catch (Exception ex)
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
                            dtB.Rows.Add(parts[0], parts[1], parts[2], parts[3], parts[4], 
                                       parts[5], parts[6], parts[7], parts[8]);
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
                        dgvPositionsA.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvPositionsA.Columns)
                        {
                            col.Visible = true;
                        }
                    }
                    if (dgvPositionsB.Columns.Count > 0)
                    {
                        dgvPositionsB.AutoGenerateColumns = true;
                        dgvPositionsB.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvPositionsB.Columns)
                        {
                            col.Visible = true;
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
            dtA.Columns.Add("备注", typeof(string));

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
                            dtA.Rows.Add(parts[0], parts[1], parts[2], parts[3], parts[4], 
                                       parts[5], parts[6], parts[7], parts[8]);
                        }
                        else if (parts.Length >= 8)
                        {
                            // 旧格式兼容：只有平仓价，开仓价显示为"-"
                            dtA.Rows.Add(parts[0], parts[1], parts[2], parts[3], parts[4], 
                                       "-", parts[5], parts[6], parts[7]);
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
            dtB.Columns.Add("备注", typeof(string));

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
                            dtB.Rows.Add(parts[0], parts[1], parts[2], parts[3], parts[4], 
                                       parts[5], parts[6], parts[7], parts[8]);
                        }
                        else if (parts.Length >= 8)
                        {
                            // 旧格式兼容：只有平仓价，开仓价显示为"-"
                            dtB.Rows.Add(parts[0], parts[1], parts[2], parts[3], parts[4], 
                                       "-", parts[5], parts[6], parts[7]);
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
                        dgvHistoryA.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvHistoryA.Columns)
                        {
                            col.Visible = true;
                        }
                    }
                    if (dgvHistoryB.Columns.Count > 0)
                    {
                        dgvHistoryB.AutoGenerateColumns = true;
                        dgvHistoryB.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
                        // 确保所有列都可见
                        foreach (DataGridViewColumn col in dgvHistoryB.Columns)
                        {
                            col.Visible = true;
                        }
                    }
                    
                    // 设置盈亏列的颜色和格式（第8列，索引7）
                    if (dgvHistoryA.Columns.Count > 7)
                    {
                        dgvHistoryA.Columns[7].DefaultCellStyle.Format = "F2";
                        dgvHistoryA.Columns[7].DefaultCellStyle.ForeColor = System.Drawing.Color.Black;
                    }
                    if (dgvHistoryB.Columns.Count > 7)
                    {
                        dgvHistoryB.Columns[7].DefaultCellStyle.Format = "F2";
                        dgvHistoryB.Columns[7].DefaultCellStyle.ForeColor = System.Drawing.Color.Black;
                    }
                    
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
    }
}

